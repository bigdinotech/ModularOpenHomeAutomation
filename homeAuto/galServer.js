var http = require("http").createServer(onRequest),
    fs = require('fs'),
    url = require('url'),
    cheerio = require('cheerio'),
    dgram = require('dgram'),
    page = "",
    body = "",
    udp_msg="",
	socketid = 0,
	SKETCH_PORT=2000,
	WEBSERVER_PORT=2010;

// reads the html page 
var page = fs.readFileSync('homeAuto.html').toString()

// reads the image (static)
var img = fs.readFileSync('./galileo.jpg').toString("base64");
     
// using cheerio to transverse the page	 
var $ = cheerio.load(page);
    $('img').attr('src','data:image/jpg;base64,'+img);
	
// getting the html string  
page = $.html();

//
//  UDP server 
//

var server = dgram.createSocket("udp4");

// this 'message' event receives the sketch datagram
server.on("message", function (msg, rinfo) {
  
    udp_msg = msg.toString();
    console.log("from " +
                 rinfo.address + " message:" + udp_msg);
	
    // just bypassing the message sent by the sketch
    io.emit("server-event-info", udp_msg);	
	
});

// this is to bind the socket
server.on("listening", function () {
  var address = server.address();
  console.log("server listening " +
      address.address + ":" + address.port);
});

server.bind(WEBSERVER_PORT);


//
// This function is to hash the response
//
hash4me = function(data){
    var firstSplits = data.split('&'),
        finalHash = [];

    // scanning first list
    for (i = 0; i < firstSplits.length; i++)
    {
        var lastSplits = firstSplits[i].split('=');
        finalHash[lastSplits[0]] = lastSplits[1];
    }
    return finalHash;
}

//
// Checking the GET and POST methods and 
// respective responses
//

function onRequest(request, response) {
  console.log("Request received.");

  var url_parts = url.parse(request.url,true);

//
// GET methods
//
  if (request.method == 'GET') {
     console.log('Request found with GET method');
     
     request.on('data',function(data)
       { response.end(' data event: '+data);
       });
    
     if(url_parts.pathname == '/')
         // when this message is displayed your browser
         // will be able to read the html page.		 
         console.log('Showing the home.html');		 
         response.end(page);    
  }
 
//
//  POST methods
//
  else if (request.method == 'POST') {

       // the post we need to parse the L1 and L2
	   // and assembly a nice message that will be received by
	   // sketch UDP server
       console.log('Request found with POST method');     
	   
	    // handling data received
        request.on('data', function (data) {
            body = "";
            body += data;
            console.log('got data:'+data);


        });
		
		
        request.on('end', function () {
 
            var message = new Buffer(20); 
			
			message.fill(0);
			
            if (body != '') {
			
                var command = "";
			
			    // dividing the commands to understand the state of each one
				// note in the radio buttons L1 and L2 the parameter "checked" 
				// must be removed. However, we are removing twice because there
				// is a bug. Some versions of node.js and cheerio even when you 
				// remove the item checked="checked", sometimes the tag checked
                // remains in the html element and the browser confuses
                //				
				// $(the element).attr("checked", null);
				// $(the element).removeAttr("checked");
				
			    var hash = hash4me(body);
                if (hash["l1"] == "0") {

                  console.log("LAMP 1 is OFF");
				   $('input[name="l1"][value="0"]').attr("checked", "checked");				   
				   $('input[name="l1"][value="1"]').attr("checked", null);
				   $('input[name="l1"][value="1"]').removeAttr("checked");
                                
                   // command message								
                   message.write("L1OFF&");                
                } else if (hash["l1"] == "1") {
                  console.log("LAMP 1 is ON");
		           $('input[name="l1"][value="0"]').attr("checked", null);
				   $('input[name="l1"][value="0"]').removeAttr("checked");
				   $('input[name="l1"][value="1"]').attr("checked", "checked");
				   
                   // command message								
                   message.write("L1ON &");				   
                }

				console.log("len:"  + message.toString().length);
                if (hash["l2"] == "0") {

                  console.log("LAMP 2 is OFF");
				   $('input[name="l2"][value="0"]').attr("checked", "checked");
				   $('input[name="l2"][value="1"]').attr("checked", null);	
                   $('input[name="l2"][value="1"]').removeAttr("checked");

                   // command message								
                   message.write("L2OFF",  6);
				   
                } else if (hash["l2"] == "1") {
                  console.log("LAMP 2 is ON");
				   $('input[name="l2"][value="0"]').attr("checked", null);
				   $('input[name="l2"][value="0"]').removeAttr("checked");
				   $('input[name="l2"][value="1"]').attr("checked", "checked");		

                   // command message								
                   message.write("L2ON ",  6);				   
                }
				
				
				// informing sketch about the changes
				// this is the message sent from webserver to sketch
                server.send(message, 0, message.length, SKETCH_PORT, "localhost", function(err, bytes) {
								 
								 if (err) {
								    console.log("Ops... some error sending UDP datagrams:"+err);
								    throw err;
                                 }                                     
                            });

				body = "";
			}

		   // update the page with the command
		   response.writeHead(200);
           response.end($.html());
        });
  }
}


// declaring the socket.io server using the "http"
var io = require('socket.io').listen(http);

// http will listen in port 8080
http.listen(8080);

// TCP socket

io.sockets.on('connection', function (socket) {

  socketid =  socket.id;
  
  socket.on('client-event', function (data) {
    console.log('just to debug the connection done, ' + data.name);
  });
});

console.log("Home automation server running...");
