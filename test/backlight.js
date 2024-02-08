const SerialPort = require('serialport').SerialPort ;
const { ReadlineParser } = require('@serialport/parser-readline') ;
const port = new SerialPort({path: '/dev/serial0', 
  baudRate: 115200
});
const parser = port.pipe(new ReadlineParser({ delimiter: '\r\n' })) ;


var counter = 0;
var strands = [] ;
var writebuffer = [] ;

var ready_to_receive = false ;
var overlayId = 0 ;

function print() {
	console.log("STRAND = \n",JSON.stringify(strands,undefined,2));
};

function create_strands(no_strands, no_pixels_list) {
	var i;
	no_pixels_list = Array.isArray(no_pixels_list)?no_pixels_list:[no_pixels_list];
	if (no_pixels_list.length != no_strands) { return ; } 
	for (i = no_strands-1 ; i>= 0 ; i--) {
		var strandobject = {
			pLayers: [{ id : -1 , pixels : new Array(no_pixels_list[i])}],
			};
//	    console.log("DEBUG: length = ", strandobject.pLayers[0].pixels.length);
		strands.push(strandobject) ;
	}
};

function set_permanent(strand_no, pixels) {
	var index = 0 ;
	pixels = Array.isArraypixels?pixels:[pixels] ;
	var repeat = pixels.length ;
	for (var i = 0 ; i < strands[strand_no-1].pLayers[0].pixels.length ; i++) {
		strands[strand_no-1].pLayers[0].pixels[i] = pixels[index] ;
		index = (index + 1) % repeat ;
	};
};


function push_overlay(strand_no, pixelList) {
		var n = strands[strand_no-1].pLayers.length ;
		strands[strand_no-1].pLayers[n] = { id : -1 , pixels: [] } ;
//		console.log("Insert pixellist:", pixelList) ;
		for ( var tbs of pixelList) {
			strands[strand_no-1].pLayers[n].pixels[tbs.n] = {r : tbs.r, g: tbs.g, b: tbs.b} ;
			};
		overlayId = (overlayId + 1) % 10000;
		strands[strand_no-1].pLayers[n].id = overlayId;
		return overlayId ;	
};


function remove_overlay(strand_no, id) {
	for (var i = 1 ; i < strands[strand_no-1].pLayers.length ; i++) {
		if (strands[strand_no-1].pLayers[i].id == id) {
			strands[strand_no-1].pLayers.splice(i,1);	
			return ;
		}
	}
};


function show (strand_no, timer) {
	if (timer === undefined) {timer = 0};
	var compactPixelList = [] ;
	for (var i = 0 ; i < strands[strand_no-1].pLayers[0].pixels.length ; i++ ) {
		for (var j = 0 ; j < strands[strand_no-1].pLayers.length ; j++) {
			if ( strands[strand_no-1].pLayers[j].pixels[i] ) {
				var pixel = strands[strand_no-1].pLayers[j].pixels[i] ;
				compactPixelList[i] = (pixel.r << 16) + (pixel.g << 8) + (pixel.b) 
			};
		}
	}

	var showObject = {command: 0,
		strand : strand_no ,
		pixels : compactPixelList ,
		timer : timer
		};
	console.log("PI PICO TO>>>");
	console.log(counter.toString() + JSON.stringify(showObject));
	console.log("<<<");
	my_port_write(JSON.stringify(showObject));
	counter++;
};
	
port.on('open', function() {
  console.log("port succesfully opened");
});

parser.on('data', function(data) {
	if (data.search("lux") != -1 || 
	    data.search("brightness") != -1 ||
	    data.search("FULL") != -1) {
	console.log("RECEIVED '"+ data + "' from PICO");};
	if (data = ">") {
		ready_to_receive = true ;
	}
});


function my_port_write(str) {
	if (writebuffer.length >30) {
		console.log("WRITEBUFFER TO PICO ERROR (BUFFER FULL)");
		return;
	};
	writebuffer[writebuffer.length] = str + "\r\n";
	emptyBuffer();
};

function emptyBuffer() {
//	console.log("DEBUG: empty buffer bufferlength = ", writebuffer.length, ", ready_to_receive =", ready_to_receive);
	if (writebuffer.length != 0) {
		if (ready_to_receive != true) {	
			setTimeout ( emptyBuffer , 200);
			return;
		};
		var firstline = writebuffer.shift() ;
		writeAndDrain(firstline,emptyBuffer);
	};
};

function writeAndDrain (data, callback) {
	//mark that pi pico is not ready to receive more
	ready_to_receive = false ;
	port.write(data, function (error) { 
		if(error){console.log(error);}
		else{
			port.drain(callback);      
		}
	});
}

create_strands(2,[1,1]);
set_permanent(1,{r:255,g:165,b:79}); 
set_permanent(2,{r:255,g:165,b:79}); 
show(1) ;
show(2) ;

var color = [ {n: 0,r:255,g:0, b:0}, {n: 0,r:0,g:255, b:0}, {n: 0,r:0,g:0, b:255}] ;
var counter2 = 0 ;
background = true ;
var o = [];

function do_beautiful() {
	if (counter2 < 3) {
		o.push(push_overlay(1,[color[counter2]]));
	    o.push(push_overlay(2,[color[counter2]]));
		background = false ;
//		console.log("overlay-id = ", o[o.length - 1]);
	} else {
		remove_overlay(2,o.pop());
		remove_overlay(1,o.pop());
		background = true ;
	};
	counter2 = (counter2 + 1 ) % 6 ;
//	console.log("DEBUG: counter = ", counter2, "background = ", background) ;
	show(1,2000) ;
	show(2,2000);
};

do_beautiful() ;
setInterval( do_beautiful, 6000) ;
