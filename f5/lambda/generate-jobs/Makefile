lambda:
	GOOS=linux go build -o f5_generate_jobs .
	zip f5-generate-jobs.zip -r ./f5_generate_jobs

clean:
	rm -f f5-generate-jobs.zip f5_generate_jobs
