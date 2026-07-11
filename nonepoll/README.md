Here in the while loop we see that if we open nc localhost 9090 in two terminals, even then only one of them is accepted, the other one is queued in the kernal

client A - nc localhost 9090 (terminal 1)
client B - nc localhost 9090 (terminal 2)
type something in terminal 2 (server doesnot react, input sent is queued in the kernal)

type soemthing in terminal 1
server prints received from terminal 1
server prints received from terminal 2 and closes immeditely

this is an example of a blocking call

untill client A is closed we cannot move on to the client B.

so I am thinking what if i spin multiple worker threads and hand off the receive to them so that the main can handle connections and for the receiving and printing part it can handover the job to worker threads.

The thread-per-connection model works because the main thread can keep accepting clients while each worker thread blocks in recv(). However, for 10,000 clients this becomes expensive because we create 10,000 OS threads. Each thread consumes memory (stack, thread metadata) and adds scheduling/context-switch overhead, even if the client is mostly idle.

And the socket state and all are stored in the kernel.


Non blocking accept and recv

when we do non blocking accept  (done by fcntl and makeing the fd O_NONBLOCK)

then when no connections are available then the accept returns with an error, if there is a connection then its queued in the kernel, when we keep polling in a loop then we will accept the connection as soon as it is available. 

the same with recv, instead of blocking recv if we use non blocking recv we have to poll it to get the data, if there are multiple clients we can accept them and put them in a vector and loop on them and check if some data is received. This is basically polling and checking all the client file descriptors to see if data is received, but polling takes up cpu and system resources.
