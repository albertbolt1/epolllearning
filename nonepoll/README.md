Here in the while loop we see that if we open nc localhost 9090 in two terminals, even then only one of them is accepted, the other one is queued in the kernal

client A - nc localhost 9090 (terminal 1)
client B - nc localhost 9090 (terminal 2)
type something in terminal 2 (server doesnot react, input sent is queued in the kernal)

type soemthing in terminal 1
server prints received from terminal 1
server prints received from terminal 2 and closes immeditely

this is an example of a blocking call

untill client A is closed we cannot move on to the client B.

so I am thinking what if i spin multiple worker threads and hand off the receive to them so that the main can handle connections and for the receiving and printing part it can handover the job to worker threads
