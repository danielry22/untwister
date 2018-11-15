package main

/*
	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

import (
	"context"
	"crypto/rand"
	"encoding/hex"
	"encoding/json"
	"fmt"
	"log"
	"os"
	"strconv"

	"github.com/aws/aws-lambda-go/lambda"
	"github.com/aws/aws-sdk-go/aws"
	"github.com/aws/aws-sdk-go/aws/session"
	"github.com/aws/aws-sdk-go/service/sqs"
)

const (
	defaultBlockSize         = 100000 // Hash computations
	queuePrefix              = "f5_"
	sqsBatchSendLimit        = 10
	defaultVisibilityTimeout = 60 * 30
)

var (
	maxPRNGSeedSize = map[string]int{
		"glibc-rand":  0xffffffff,         // UINT_MAX
		"java":        0x7fffffffffffffff, // LLONG_MAX
		"mt19937":     0xffffffff,         // UINT_MAX
		"php-mt_rand": 0xffffffff,         // UINT_MAX
		"ruby-rand":   0xffffffff,         // UINT_MAX
	}
)

// UntwisterJob -
type UntwisterJob struct {
	JobID        string `json:"job_id"`
	Observations []int  `json:"observations"`
	PRNG         string `json:"prng"`
	Depth        int    `json:"depth"`
}

// UntwisterBlock -
type UntwisterBlock struct {
	JobID        string `json:"job_id"`
	Observations []int  `json:"observations"`
	PRNG         string `json:"prng"`
	Depth        int    `json:"depth"`
	MinSeed      int    `json:"min_seed"`
	MaxSeed      int    `json:"max_seed"`
}

// UntwisterBlockInfo -
type UntwisterBlockInfo struct {
	Blocks  int `json:"blocks"`
	Batches int `json:"batches"`
}

func main() {
	lambda.Start(GenerateJobs)
}

// GenerateJobs - Generates Untwister job queues
func GenerateJobs(ctx context.Context, job UntwisterJob) (string, error) {
	executeJob(job)
	return "Started job", nil
}

func executeJob(job UntwisterJob) {

	jobLogf(job, "Start job for '%s' with depth of '%d' ...", job.PRNG, job.Depth)

	sess, _ := session.NewSession(&aws.Config{
		Region: aws.String(getTargetAWSRegion())},
	)
	sqsSrv := sqs.New(sess)

	// To make a FIFO queue you appearently just end the name with ".fifo" ...
	queue, err := createSQSQueue(job, sqsSrv, fmt.Sprintf("%s%s.fifo", queuePrefix, job.JobID))
	if err != nil {
		jobLogf(job, "Fatal error no queue")
		return
	}

	blockSize := getBlockSize()
	minSeed := 0
	maxSeed := blockSize
	sent := 0
	blocks := 0
	maxPRNGSeed := maxPRNGSeedSize[job.PRNG]

	var sqsBuffer []*sqs.SendMessageBatchRequestEntry

	for minSeed < maxPRNGSeed {
		block := UntwisterBlock{
			JobID:        job.JobID,
			Observations: job.Observations,
			PRNG:         job.PRNG,
			Depth:        job.Depth,
			MinSeed:      minSeed,
			MaxSeed:      maxSeed,
		}
		msg, _ := json.Marshal(block)
		// jobLogf(job, "Block %d -> %d", block.Skip, block.Skip+block.Limit)

		sqsBuffer = append(sqsBuffer, &sqs.SendMessageBatchRequestEntry{
			Id:                     aws.String(generateRandomID()), // Not sure what this does
			MessageBody:            aws.String(string(msg)),
			MessageGroupId:         aws.String(generateRandomID()),
			MessageDeduplicationId: aws.String(generateRandomID()),
		})

		if len(sqsBuffer) == sqsBatchSendLimit {
			blocks += len(sqsBuffer)
			sendBatchSQSMessage(job, sqsSrv, queue.QueueUrl, sqsBuffer)
			sent++
			sqsBuffer = []*sqs.SendMessageBatchRequestEntry{}
		}

		minSeed = maxSeed
		maxSeed += blockSize
	}
	if len(sqsBuffer) != 0 {
		blocks += len(sqsBuffer)
		sendBatchSQSMessage(job, sqsSrv, queue.QueueUrl, sqsBuffer)
		sent++
	}

	jobLogf(job, "Sent %d blocks in %d messages", blocks, sent)
	data, _ := json.Marshal(UntwisterBlockInfo{
		Blocks:  blocks,
		Batches: sent,
	})
	err = s3Write([]string{job.JobID}, "block-info.json", data)
	if err != nil {
		jobLogf(job, "S3 write fialed %v", err)
	}

}

func createSQSQueue(job UntwisterJob, sqsSrv *sqs.SQS, name string) (*sqs.CreateQueueOutput, error) {
	jobLogf(job, "Creating SQS queue with name: %s", name)
	result, err := sqsSrv.CreateQueue(&sqs.CreateQueueInput{
		QueueName: aws.String(name),
		Attributes: map[string]*string{
			"VisibilityTimeout":      aws.String(getVisibilityTimeout()),
			"MessageRetentionPeriod": aws.String("1209600"), // 14 days
			"FifoQueue":              aws.String("true"),
		},
	})
	if err != nil {
		jobLogf(job, "Error creating queue: %v", err)
		return &sqs.CreateQueueOutput{}, err
	}
	return result, nil
}

func sendSQSMessage(job UntwisterJob, sqsSrv *sqs.SQS, queueURL *string, message string) {
	_, err := sqsSrv.SendMessage(&sqs.SendMessageInput{
		MessageBody:            aws.String(message),
		QueueUrl:               queueURL,
		MessageGroupId:         aws.String(generateRandomID()),
		MessageDeduplicationId: aws.String(generateRandomID()),
	})
	if err != nil {
		jobLogf(job, "Error in SQS send: %v", err)
	}
}

func sendBatchSQSMessage(job UntwisterJob, sqsSrv *sqs.SQS, queueURL *string, entries []*sqs.SendMessageBatchRequestEntry) {
	// jobLogf(job, "Flushing SQS buffer with %d messages", len(entries))
	_, err := sqsSrv.SendMessageBatch(&sqs.SendMessageBatchInput{
		Entries:  entries,
		QueueUrl: queueURL,
	})
	if err != nil {
		jobLogf(job, "Error in batch SQS send: %v", err)
	}
}

func jobLogf(job UntwisterJob, pattern string, args ...interface{}) {
	log.Printf("[%s] %s", job.JobID, fmt.Sprintf(pattern, args...))
}

func getBlockSize() int {
	blockSize, err := strconv.Atoi(os.Getenv("F5_BLOCK_SIZE"))
	if err != nil {
		return defaultBlockSize
	}
	return blockSize
}

func getVisibilityTimeout() string {
	visibilityTimeout, err := strconv.Atoi(os.Getenv("F5_VISIBILITY_TIMEOUT"))
	if err != nil {
		return strconv.Itoa(defaultVisibilityTimeout)
	}
	return strconv.Itoa(visibilityTimeout)
}

func generateRandomID() string {
	bytes := make([]byte, 16)
	rand.Read(bytes) // wcgw?
	return hex.EncodeToString(bytes)
}

func getTargetAWSRegion() string {
	targetRegion := os.Getenv("F5_TARGET_AWS_REGION")
	if targetRegion != "" {
		return targetRegion
	}
	return os.Getenv("AWS_REGION")
}
