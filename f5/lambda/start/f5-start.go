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
	"crypto/rand"
	"encoding/hex"
	"encoding/json"
	"errors"
	"fmt"
	"log"
	"os"

	"github.com/aws/aws-lambda-go/events"
	"github.com/aws/aws-lambda-go/lambda"
	"github.com/aws/aws-sdk-go/aws"
	"github.com/aws/aws-sdk-go/aws/session"

	// There's two lambda imports so watch out for the name collision
	lambdaSrv "github.com/aws/aws-sdk-go/service/lambda"
)

const (
	defaultGenerateJobFuncName = "f5_generate_jobs"
	queuePrefix                = "f5_"
)

var (
	errNoBody            = errors.New("no http body")
	errFailedToParseBody = errors.New("failed to parse http body")

	errStepFunction = errors.New("error calling step function")

	errUnsupportedPRNG = errors.New("unsupported prng")
	supportedPRNGs     = map[string]bool{
		"glibc-rand":  true,
		"java":        true,
		"mt19937":     true,
		"php-mt_rand": true,
		"ruby-rand":   true,
	}
)

// LambdaError - Error mapped to JSON
type LambdaError struct {
	Error string `json:"error"`
}

// UntwisterJobReq -
type UntwisterJobReq struct {
	Observations []int  `json:"observations"`
	PRNG         string `json:"prng"`
	Depth        int    `json:"depth"`
}

// UntwisterJob -
type UntwisterJob struct {
	JobID        string `json:"job_id"`
	Observations []int  `json:"observations"`
	PRNG         string `json:"prng"`
	Depth        int    `json:"depth"`
}

func main() {
	lambda.Start(RequestHandler)
}

// RequestHandler - Handle an HTTP request
func RequestHandler(request events.APIGatewayProxyRequest) (events.APIGatewayProxyResponse, error) {
	log.Printf("Processing Lambda request %s\n", request.RequestContext.RequestID)

	// If no name is provided in the HTTP request body, throw an error
	if len(request.Body) < 1 {
		return JSONError(errNoBody), nil
	}

	log.Printf("Parsing request body ...")
	var req UntwisterJobReq
	err := json.Unmarshal([]byte(request.Body), &req)
	if err != nil {
		log.Printf("Failed to parse request: %v", err)
		return JSONError(errFailedToParseBody), nil
	}

	if !isSupportedPRNG(req.PRNG) {
		return JSONError(errUnsupportedPRNG), nil
	}

	job := UntwisterJob{
		JobID:        generateJobID(),
		Observations: req.Observations,
		Depth:        req.Depth,
		PRNG:         req.PRNG,
	}

	payload, _ := json.Marshal(job)
	sess := session.Must(session.NewSessionWithOptions(session.Options{
		SharedConfigState: session.SharedConfigEnable,
	}))

	client := lambdaSrv.New(sess, &aws.Config{Region: aws.String(os.Getenv("AWS_REGION"))})

	stepFunc := getGenerateJobFuncName()
	log.Printf("Executing step function: %s", stepFunc)

	_, err = client.Invoke(&lambdaSrv.InvokeInput{
		InvocationType: aws.String("Event"),
		FunctionName:   aws.String(stepFunc),
		Payload:        payload,
	})
	if err != nil {
		log.Printf("Error calling '%s' step function (%v)", stepFunc, err)
		return JSONError(errStepFunction), nil
	}

	return events.APIGatewayProxyResponse{
		Body:       string(payload),
		StatusCode: 200,
	}, nil
}

func getGenerateJobFuncName() string {
	generateJobFunc := os.Getenv("F5_GENERATE_JOBS")
	if len(generateJobFunc) < 1 {
		return defaultGenerateJobFuncName
	}
	return generateJobFunc
}

func isSupportedPRNG(prng string) bool {
	_, ok := supportedPRNGs[prng]
	return ok
}

func generateJobID() string {
	bytes := make([]byte, 16)
	rand.Read(bytes) // wcgw?
	return hex.EncodeToString(bytes)
}

// JSONError - Convert an error to a valid API Response
func JSONError(err error) events.APIGatewayProxyResponse {
	msg, _ := json.Marshal(LambdaError{
		Error: fmt.Sprintf("%v", err),
	})
	return events.APIGatewayProxyResponse{
		StatusCode: 400,
		Body:       string(msg),
	}
}
