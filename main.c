/*
  Copyright (c) 2013 Alberto Glarey
 
  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:
 
  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.
 
  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
  THE SOFTWARE.
*/

//Standard include files
#include "stdlib.h"
#include "stdio.h"
#include "string.h"

/*
zeromq library : www.zeromq.org
NOTE : on errors, the zmq functions also sets errno to a defined value
It is possible to retrieve the error message string calling zmq_strerror(errno)
*/
#include "zmq.h"

//zlib library : www.zlib.net
#include "zlib.h"

//cJSON library by Dave Gamble : www.sourceforge.net/projects/cjson/
#include "cJSON.h"

//The max size in bytes allowed to allocate space for uncompressed data (1M)
#define MAX_BUFFER_SIZE 1048576

//The desired relay server address
#define SERVER_ADDRESS "tcp://relay-us-west-1.eve-emdr.com:8050"

/*
 * MAIN
 */
int main(int argc,char **argv)
{
	//The required variables to handle the zmq lib
	void *z_context = NULL;
	void *z_socket = NULL;
	zmq_msg_t z_msg;

	//The required data buffers and size indicators to hold and expand the received zmq messages
	unsigned char *compressed_msg = NULL;
	int compressed_msg_size = 0;
	unsigned char *uncompressed_msg = NULL;
	int uncompressed_msg_size = 0;

	//The required variables to handle the json objects
	cJSON *json = NULL;
	char *out = NULL;

	//Create a new zmq context, return NULL on errors
	z_context = zmq_ctx_new();

	//Create the zmq socket, return NULL on errors
	z_socket = zmq_socket(z_context,ZMQ_SUB);

	//Disable filtering, return zero if successful, -1 on errors
	zmq_setsockopt(z_socket,ZMQ_SUBSCRIBE,"",0);

	//Connect to the desired relay, return zero if successful, -1 on errors
	zmq_connect(z_socket,SERVER_ADDRESS);

	//Infinite loop
	while(1)
	{
		//Initialize the  zmq message, return zero if successful, -1 on errors
		zmq_msg_init(&z_msg);

		//Receive a zmq message, return the number of bytes received if successful, -1 on errors
		compressed_msg_size = zmq_msg_recv(&z_msg,z_socket,0);

		/*
		Allocate a buffer and copy the compressed data received in the zmq message
		The zmq documentation explicitly say :
		"Never access zmq_msg_t members directly, instead always use the zmq_msg(...) family of functions"
		*/
		compressed_msg = (unsigned char *)malloc(compressed_msg_size*sizeof(unsigned char));
		memcpy(compressed_msg,zmq_msg_data(&z_msg),compressed_msg_size);

		//Allocate a buffer where to hold the uncompressed data
		uncompressed_msg_size = MAX_BUFFER_SIZE;
		uncompressed_msg = (unsigned char *)malloc(uncompressed_msg_size*sizeof(unsigned char));

		/*
		The zlib function uncompress(...) return Z_OK if successful, on errors it can return :
		- Z_BUF_ERROR : The buffer destination buffer was not large enough to hold the uncompressed data
		- Z_MEM_ERROR : Insufficient memory
		- Z_DATA_ERROR : The compressed data was corrupted
		*/
		uncompress(uncompressed_msg,(uLongf *)&uncompressed_msg_size,compressed_msg,compressed_msg_size);
		//NOTE : After the call to this function, uncompressed_msg hold the uncompressed data, uncompressed_msg_size hold the size of uncompressed_msg

		/*
		Here is where the "magic" happen...
		Parse the uncompressed buffer data in a root json object, return 0 on errors
		It is possible to call cJSON_GetErrorPtr() that return a pointer to the parse error
		*/
		json = cJSON_Parse((char*)uncompressed_msg);

		//Print out the market data. Or, you know, do more fun things here.
		out = cJSON_Print(json);
		printf("%s\n",out);

		//Clean the printed string and free the json object with all subentities
		free(out);
		cJSON_Delete(json);

		//Free the data buffers
		free(uncompressed_msg);
		free(compressed_msg);

		//Close the zmq message, return zero if successful, -1 on errors
		zmq_msg_close(&z_msg);
	};

	/*
	NOTE : this point is after an infinite loop while(1) and it will be never reached
	The following code is only for educative purpose
	*/

	//Close the zmq socket, return zero if successful -1 on errors
	zmq_close(z_socket);

	//Free the zmq context, return zero if successful -1 on errors
	zmq_ctx_destroy(z_context);

	//Exit succesful
	return 0;
}