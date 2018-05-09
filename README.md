This project is a sample of implementation of Loebner Prize Protocol 2 ( https://github.com/jhudsy/LoebnerPrizeProtocol ) in C++.

# Getting Started
Download the project and compile it with a C++ compilator on Windows (SEVEN minimum). This project have ben tested with Visual Studio Express and code::block, but there is nothing specific to a compilator.

## Generalities
The WebSocket module implements a Web Socket IO to communicate with the Judge program of Loebner Protocol via internet. The LoebnerInterface module manages the Loebner protocol-specific part and exchanges with the Web Socket. The LoebnerBot module manages the HMI dialog with the user on one hand, and the interface with the chatbot on the other hand:

JUDGE <==> internet <==> WebSocket <==> LoebnerInterface <==> LoebnerBot <==> Your bot


The bot showed in example here only repeats the questions of the judge. I'm not sure it will pass the Turing test. Then you will need to adapt it to your chatbot.

## Adaptation to your chatbot
- Step 1: Create a nice icon and put it in the place of the existing icon.
- Step 2: Customize the title of the dialog box.
- Step 3: Program the launch of your chatbot when a round is going to start.
- Step 4: Program the stop of your chatbot when a round is over.
- Step 5: Program the sending of judge's questions to your chatbot, and your chatbot's response to the judge.
- Step 6: Change parameters MIN_SPLIT_COMMA, MIN_SPLIT_POINT and BOT_SPEED to have a "human like" behaviour.

Steps are noted "TODO" in program, search it to find the parts you have to change.

# Troubleshooting
If something goes wrong, you can activate a trace file. Set the variable "TRACE" in WebSocket.cpp. A trace file named LOGXXXX.tmp will be created in temporary folder.
This file lists all exchanges between the program and the server like this, for example:

---------- CLIENT ----------  
GET /socket.io/?EIO=3&transport=polling&t=cv HTTP/1.1  
Accept: */*  
Accept-Language: en-EN  
Accept-Encoding: gzip, deflate  
User-Agent: Mozilla/5.0 (Windows NT 6.1; WOW64; Trident/7.0; rv:11.0) like Gecko  
Host: 127.0.0.1:8080  
DNT: 1  
Connection: Keep-Alive  
Cookie: io=pTzblLqqYY3Vn6w5AAAm  
  
  
---------- SERVER ----------  
HTTP/1.1 200 OK  
Content-Type: application/octet-stream  
Content-Length: 101  
Access-Control-Allow-Origin: *  
X-XSS-Protection: 0  
Set-Cookie: io=9O32aGc7kkFmpAhkAAAJ; Path=/; HttpOnly  
Date: Tue, 08 May 2018 21:15:42 GMT  
Connection: keep-alive  
  
---------- CLIENT ----------  
GET /socket.io/?EIO=3&transport=websocket&sid=9O32aGc7kkFmpAhkAAAJ HTTP/1.1  
Origin: null  
Sec-WebSocket-Key: AJAxz5V6WeXSvyzbtYNNAw==  
Sec-WebSocket-Version: 13  
User-Agent: Mozilla/5.0 (Windows NT 6.1; WOW64; Trident/7.0; rv:11.0) like Gecko  
Host: 127.0.0.1:8080  
DNT: 1  
Connection: Upgrade  
Upgrade: Websocket  
Cache-Control: no-cache  
Cookie: io=9O32aGc7kkFmpAhkAAAJ  
  
  
---------- SERVER ----------  
HTTP/1.1 101 Switching Protocols  
Upgrade: websocket  
Connection: Upgrade  
Sec-WebSocket-Accept: kAwW+2aVwI/YYUoVwSe0Zlg+9mI=  
  
  
---------- CLIENT ----------  
8186175D252A252D57457538  
OpCode = 1 masked 2probe  
  
---------- SERVER ----------  
81063370726F6265  
OpCode = 1 not masked 3probe  
  
---------- CLIENT ----------  
8181FD721E01C8  
OpCode = 1 masked 5  
  
---------- SERVER ----------  
81023430  
OpCode = 1 not masked 40  
  
---------- CLIENT ----------  
81CE026088923652D3B0610FE6E6700FE4B02E42F3CE2013FCF37615FBCE205AD4B07005EFFB7114EDE05E42A4CE2009ECCE205AD4B06309B8CE204CD4B07105EBE06714D4B0383CAAF36003B9A0313CAAEF203D  
OpCode = 1 masked 42["control","{\"status\":\"register\",\"id\":\"ai0\",\"secret\":\"abc123\"}"]  
  
---------- CLIENT ----------  
81D69A9B2AA9AEA9718BF9F444DDE8F4468BB6B951F5B8E85EC8EEEE59F5B8A1768BE8F45FC7FED244CFF5E947C8EEF245C7C6B906F5B8F24EF5B8A1768BFBF21AF5B8B7768BE9FE49DBFFEF768BA0C708C8F8F81B9BA9C708D4B8C6  
OpCode = 1 masked 42["control","{\"status\":\"roundInformation\",\"id\":\"ai0\",\"secret\":\"abc123\"}"]  
  
---------- SERVER ----------  
817E018034325B22726F756E64496E666F726D6174696F6E222C227B5C22726F756E644E756D6265725C223A302C205C227374617475735C223A5C2252756E6E696E675C222C205C22706172746E6572735C223A7B5C226A75646765305C223A5B5C22636F6E66305C222C5C226169305C225D2C5C226A75646765315C223A5B5C22636F6E66315C222C5C226169315C225D2C5C226A75646765325C223A5B5C22636F6E66325C222C5C226169325C225D2C5C226A75646765335C223A5B5C22636F6E66335C222C5C226169335C225D2C5C226169305C223A5B5C226A75646765305C225D2C5C226169315C223A5B5C226A75646765315C225D2C5C226169325C223A5B5C226A75646765325C225D2C5C226169335C223A5B5C226A75646765335C225D2C5C22636F6E66305C223A5B5C226A75646765305C225D2C5C22636F6E66315C223A5B5C226A75646765315C225D2C5C22636F6E66325C223A5B5C226A75646765325C225D2C5C22636F6E66335C223A5B5C226A75646765335C225D7D7D225D  
OpCode = 1 not masked 42["roundInformation","{\"roundNumber\":0, \"status\":\"Running\", \"partners\":{\"judge0\":[\"conf0\",\"ai0\"],\"judge1\":[\"conf1\",\"ai1\"],\"judge2\":[\"conf2\",\"ai2\"],\"judge3\":[\"conf3\",\"ai3\"],\"ai0\":[\"judge0\"],\"ai1\":[\"judge1\"],\"ai2\":[\"judge2\"],\"ai3\":[\"judge3\"],\"conf0\":[\"judge0\"],\"conf1\":[\"judge1\"],\"conf2\":[\"judge2\"],\"conf3\":[\"judge3\"]}}"]  
