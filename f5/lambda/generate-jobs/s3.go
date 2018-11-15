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
	"bytes"
	"log"
	"os"
	"path"

	"github.com/aws/aws-sdk-go/aws"
	"github.com/aws/aws-sdk-go/aws/session"
	"github.com/aws/aws-sdk-go/service/s3"
	"github.com/aws/aws-sdk-go/service/s3/s3manager"
)

const (
	defaultS3BucketName = "f5_untwister"
)

func getS3BucketName() string {
	s3BucketName := os.Getenv("F5_S3_BUCKET_NAME")
	if len(s3BucketName) < 1 {
		return defaultS3BucketName
	}
	return s3BucketName
}

func s3List(folder []string) ([]string, error) {
	sess := session.Must(session.NewSessionWithOptions(session.Options{
		SharedConfigState: session.SharedConfigEnable,
	}))
	s3Srv := s3.New(sess)
	resp, err := s3Srv.ListObjects(&s3.ListObjectsInput{
		Bucket: aws.String(getS3BucketName()),
		Prefix: aws.String(path.Join(folder...)),
	})
	if err != nil {
		log.Printf("Error while listing bucket '%s': %v", getS3BucketName(), err)
		return []string{}, err
	}
	contents := []string{}
	for _, item := range resp.Contents {
		contents = append(contents, (*item.Key))
	}
	return contents, nil
}

func s3Write(folder []string, fileName string, contents []byte) error {
	sess := session.Must(session.NewSessionWithOptions(session.Options{
		SharedConfigState: session.SharedConfigEnable,
	}))
	uploader := s3manager.NewUploader(sess)

	root := path.Join(folder...)
	key := path.Join(root, fileName)
	log.Printf("s3Write -> %s", key)

	_, err := uploader.Upload(&s3manager.UploadInput{
		Bucket: aws.String(getS3BucketName()),
		Key:    aws.String(key),
		Body:   bytes.NewReader(contents),
	})
	return err
}

func s3Read(folder []string, fileName string) ([]byte, error) {

	sess := session.Must(session.NewSessionWithOptions(session.Options{
		SharedConfigState: session.SharedConfigEnable,
	}))
	downloader := s3manager.NewDownloader(sess)

	root := path.Join(folder...)
	key := path.Join(root, fileName)
	log.Printf("s3Read <- %s", key)

	buffer := aws.NewWriteAtBuffer([]byte{})
	_, err := downloader.Download(buffer, &s3.GetObjectInput{
		Bucket: aws.String(getS3BucketName()),
		Key:    aws.String(key),
	})
	return buffer.Bytes(), err
}
