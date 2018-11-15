const request = require("request-promise-native");
const express = require("express");
const uuidv4 = require("uuid/v4");
const AWS = require("aws-sdk");

var execFile = require("child_process").execFile;
AWS.config.setPromisesDependency(null);
const app = express();
const port = 80;

const untwister = "/opt/untwister"

var sqs = new AWS.SQS({
	"region": process.env.AWS_DEFAULT_REGION
});

var s3 = new AWS.S3({
	"region": process.env.AWS_DEFAULT_REGION
});

/*
	Simple async wait
*/
function setTimeoutAsync(milliseconds) {
	return new Promise((resolve, _) => {
		setTimeout(resolve, milliseconds);
	});
}

function shell_exec(command, args) {
	return new Promise((resolve, _) => {
		execFile(command, args, (error, stdout, stderr) => {
			resolve({
				"error": error,
				"stdout": stdout,
				"stderr": stderr
			});
		});
	});
}

async function untwister_exec(prng, inputs, depth, min_seed, max_seed) {
	console.log("Running untwister...");
	let command_result = await shell_exec(
		untwister,
		[
			"-q",
			"-b",
			"-r",
			prng,
			"-D",
			depth,
			"-s",
			min_seed,
			"-S",
			max_seed,
			"-i",
			input_file,
		]
	);

	// Check if an error occured
	if (command_result.stderr) {
		return {
			"success": false,
			"error": command_result.stderr
		}
	}

	if (command_result.error) {
		return {
			"success": true,
			"error": command_result.stderr
		}
	}

	let seed = command_result.stdout.trim();
	return {
		"success": true,
		"seed": seed,
	}
}

/*
	Get all SQS queues
*/
async function start_worker() {

	while (true) {
		console.log("Retrieving SQS queue(s)...");
		// Get all of the current queue URLs
		var sqs_result = await sqs.listQueues({
			"QueueNamePrefix": "f5_"
		}).promise();
		var queue_urls = sqs_result["QueueUrls"];

		if (!queue_urls) {
			queue_urls = [];
			console.log("No queues exist, waiting two seconds before looping...");
			await setTimeoutAsync(
				(1000 * 2)
			)
		}

		console.log("Pulling off of each queue...");
		// Pull and process messages off each queue in an RR-fashion
		for (var i = 0; i < queue_urls.length; i++) {
			console.log("Receiving messages from " + queue_urls[i]);
			var queue_messages = await sqs.receiveMessage({
				"QueueUrl": queue_urls[i],
				"AttributeNames": [
					"All",
				],
				"MaxNumberOfMessages": 1,
				"WaitTimeSeconds": 10,
			}).promise();

			// Assuming there was anything on the queue
			if ("Messages" in queue_messages && queue_messages["Messages"].length > 0) {
				// Used to confirm item has been processed
				var message_receipt_handler = queue_messages["Messages"][0]["ReceiptHandle"]
				var queue_message = JSON.parse(
					queue_messages["Messages"][0]["Body"]
				);

				// TODO

				console.log("Deleting message off of SQS...");
				// Delete message off queue
				var sqs_delete = await sqs.deleteMessage({
					"QueueUrl": queue_urls[i],
					"ReceiptHandle": message_receipt_handler,
				}).promise();

				console.log(sqs_delete);
			}
		}
	}
}

app.get("/", (_, response) => {
	response.send({
		"msg": "Hello from f5"
	});
});

app.listen(port, async function () {
	console.log(`F5 Untwister server app listening on port ${port}!`);
	while (true) {
		await start_worker().catch(error => {
			console.log(error);
			console.log("Some error occured (see above), restarting worker...");
		});
	}
});