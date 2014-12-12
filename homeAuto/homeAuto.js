var auth = require('http-auth');
var basic = auth.basic({
    realm: "Home Automation",
    file: __dirname + "/htpasswd"
});

var http = require("http").createServer(basic, onRequest),
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
page = $.html();


	
var server = dgram.createSocket("udp4");

var io = require('socket.io').listen(http);

// this 'message' event receives the sketch datagram
server.on("message", function (msg, rinfo) {
  
    udp_msg = msg.toString();
	var n = udp_msg.length;
	console.log(msg);
	console.log("length: " + n);
	//console.log(udp_msg);

	io.emit("server-event-info", udp_msg);
	
});

// this is to bind the socket
server.on("listening", function () {
  var address = server.address();
  console.log("server listening " +
      address.address + ":" + address.port);
});

server.bind(WEBSERVER_PORT);

function onRequest(request, response) {
  console.log("Request received.");

  var url_parts = url.parse(request.url,true);

//
// GET methods
//
  if (request.method == 'GET') 
  {
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
  else if (request.method == 'POST') 
  {
	var command;
	console.log("***POST***");
	// the post we need to parse the L1 and L2
	// and assembly a nice message that will be received by
	// sketch UDP server
    console.log('Request found with POST method');
	request.on('data', function (data) 
	{
        command = "";
        command += data;
        console.log('got data:'+data);
    });

	request.on('end', function () 
	{
		


	});

  }
}

http.listen(8080);

// TCP socket

io.sockets.on('connection', function (socket) {

  socketid =  socket.id;
  
  socket.on('client-event', function (data) 
  {
    console.log('just to debug the connection done, ' + data.name);
  });
  
  socket.on('command', function(data) 
  {
	console.log("command: " + data);
	var udpBuff = new Buffer(data.length)
	udpBuff.fill(0);
	udpBuff.write(data);
	console.log("udpBuff: ", udpBuff);
	server.send(udpBuff, 0, udpBuff.length, SKETCH_PORT, "localhost", function(err, bytes)
	{
		if (err) 
		{
			console.log("Ops... some error sending UDP datagrams:"+err);
			throw err;
		}
	});
	console.log("done sending");
  });
  
});

console.log("Home automation server running...");

