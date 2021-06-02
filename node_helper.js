/* Magic Mirror
 * Module: MagicMirror-LCD-Backlights

 * MIT Licensed.
 */
 
var NodeHelper = require('node_helper');
const SerialPort = require('serialport') ;
const Readline = require('@serialport/parser-readline');
const port = new SerialPort('COM4', {
  baudRate: 115200
});
const parser = port.pipe(new Readline({ delimiter: '\r\n' }));

var counter = 0;
var strands = [] ;
var writebuffer = [] ;

var ready_to_receive = false ;
var overlayId = 0 ;
const MMM_LCD_activeOverlayMAP = new Map() ;
const MMM_LCD_activePayloadMAP = new Map() ;

module.exports = NodeHelper.create({
	
  start: function () {
    console.log('MMM_LCD_Backlight helper started ...');
	port.on('open', function() {
		console.log("port succesfully opened");
	});
	parser.on('data', function(data) {
		console.log("RECEIVED '"+ data + "' from PICO");
		if (data = ">") {
			ready_to_receive = true ;
		}
	});
  },

  config: {
        nStrings: 2,  		// There are two strings
		nPixels: [30,30],		// Each string has 30 leds
        defaultColor: {r: 255, g: 167, b: 87 },   // default = warm white 2700K
        colorCommands: [], // empty list of colorcommands
		transitionTime: 5000 
  },

  print: function () {
	console.log("STRAND = \n",JSON.stringify(strands,undefined,2));
},

  create_strands: function(no_strands, no_pixels_list) {
	var i;
	no_pixels_list = Array.isArray(no_pixels_list)?no_pixels_list:[no_pixels_list];
	if (no_pixels_list.length != no_strands) { return ; } 
	for (i = no_strands-1 ; i>= 0 ; i--) {
		var strandobject = {
			pLayers: [{ id : -1 , pixels : new Array(no_pixels_list[i])}],
			};
	    console.log("DEBUG: length = ", strandobject.pLayers[0].pixels.length);
		strands.push(strandobject) ;
	}
},

  set_permanent: function (strand_no, pixels) {
	var index = 0 ;
	pixels = Array.isArraypixels?pixels:[pixels] ;
	var repeat = pixels.length ;
	for (var i = 0 ; i < strands[strand_no-1].pLayers[0].pixels.length ; i++) {
		strands[strand_no-1].pLayers[0].pixels[i] = pixels[index] ;
		index = (index + 1) % repeat ;
	};
},


  push_overlay: function(strand_no, pixelList) {
		var n = strands[strand_no-1].pLayers.length ;
		strands[strand_no-1].pLayers[n] = { id : -1 , pixels: [] } ;
//		console.log("Insert pixellist:", pixelList) ;
		for ( var tbs of pixelList) {
			strands[strand_no-1].pLayers[n].pixels[tbs.n] = {r : tbs.r, g: tbs.g, b: tbs.b} ;
			};
		overlayId = (overlayId + 1) % 10000;
		strands[strand_no-1].pLayers[n].id = overlayId;
		return overlayId ;	
},


  remove_overlay: function(strand_no, id) {
	for (var i = 1 ; i < strands[strand_no - 1].pLayers.length ; i++) {
		if (strands[strand_no-1].pLayers[i].id == id) {
			strands[strand_no-1].pLayers.splice(i,1);	
			return ;
		}
	}
},


  show: function(strand_no, timer) {
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

	var showObject = {
		strand : strand_no ,
		pixels : compactPixelList ,
		timer : timer
		};
	console.log("PI PICO TO>>>");
	console.log(counter.toString() + JSON.stringify(showObject));
	console.log("<<<");
	this.my_port_write(JSON.stringify(showObject));
	counter++;
},

  my_port_write: function(str) {
	if (writebuffer.length >30) {
		console.log("WRITEBUFFER TO PICO ERROR (BUFFER FULL)");
		return;
	};
	writebuffer[writebuffer.length] = str + "\r\n";
	this.emptyBuffer();
},

  emptyBuffer: function() {
	 var self = this ;
//	console.log("DEBUG: empty buffer bufferlength = ", writebuffer.length, ", ready_to_receive =", ready_to_receive);
	if (writebuffer.length != 0) {
		if (ready_to_receive != true) {	
			setTimeout ( self.emptyBuffer , 200);
			return;
		};
		var firstline = writebuffer.shift() ;
		self.writeAndDrain(firstline,self.emptyBuffer);
	};
},

  writeAndDrain:  function(data, callback) {
	//mark that pi pico is not ready to receive more
	ready_to_receive = false ;
	port.write(data, function (error) { 
		if(error){console.log(error);}
		else{
			port.drain(callback);      
		}
	});
},
 
   find_display: function(sender, command) {
		if (this.config.colorCommands[sender]) {
			for (let item of this.config.colorCommands[sender]) {
				if (item.command === command) {
					return [ item.strand, item.pixels, item.pixelstart ] ;
				};
			};
	   };
	   return [undefined, undefined, undefined ] ;
   },
 
   print_the_map: function(ident) {
	   console.log("MMM LCD DEBUG PRINT OVERLAY_MAP", ident);
	   for (let [key,item] of MMM_LCD_activeOverlayMAP) {
		   console.log("<",key,"> = ", JSON.stringify(item));
	   };
   },
   
  socketNotificationReceived: function(notification, payload) {
	if (notification === 'SET_LIGHTS') {
	  try{
		if (!this.config.colorCommands[payload.sender]){ return;}; //ignore non-configured modules. 
		if (MMM_LCD_activePayloadMAP.get(payload.sender) === JSON.stringify(payload)) { return ; } //ignore already processed command 
		MMM_LCD_activePayloadMAP.set(payload.sender, JSON.stringify(payload) ;
		if (payload.command == -1) {
			if (! MMM_LCD_activeOverlayMAP.has(payload.sender)) { return ;} ;
			var overlayList = MMM_LCD_activeOverlayMAP.get(payload.sender) ;
			var changed_strands = [] ;
			for (var o of overlayList) {
				this.remove_overlay(o.strand,o.id) ;
				changed_strands[o.strand] = true ;
			};
			MMM_LCD_activeOverlayMAP.delete(payload.sender);
			for (var i = 0 ; i < changed_strands.length ; i++) {
				if (changed_strands[i]) { this.show(i,this.config.transitionTime); };
			};
			return;
		};
		var strand ;
		var pixels ;
		var pixelstart;
		[strand, pixels, pixelstart] = this.find_display(payload.sender, payload.command);
		if (payload.params) {
			if (!strand) {strand = payload.params.strand;} ;
			if (!pixels) {pixels = payload.params.pixels;} ;
			if (!pixelstart) {pixelstart = payload.params.pixelstart;};
		};
		var showpix = [] ;
		for (var n = 0; n < pixels.length ; n++ ) {
			showpix.push({n: (n+pixelstart)%this.config.nPixels[strand-1], r:pixels[n].r, g:pixels[n].g, b:pixels[n].b});
		};
		var id = this.push_overlay(strand,showpix) ;
		var changed_strands = [] ;
		if (MMM_LCD_activeOverlayMAP.has(payload.sender)) {
			var overlayList = MMM_LCD_activeOverlayMAP.get(payload.sender);
			//guard against too many overlays per sender
			if (overlayList.length >= 4 ) {   // limit stacking overlays per module to 4
				for (var o of overlayList) {
					this.remove_overlay(o.strand,o.id) ;
					changed_strands[o.strand] = true ;
				};
				MMM_LCD_activeOverlayMAP.delete(payload.sender);
				MMM_LCD_activeOverlayMAP.set(payload.sender, []);
			};
			//end guard
			MMM_LCD_activeOverlayMAP.get(payload.sender).push({strand: strand, id: id});
			changed_strands[strand] = true;
		} else {
			MMM_LCD_activeOverlayMAP.set(payload.sender, [ {strand: strand, id: id} ]);
			changed_strands[strand] = true ;
		};
		for (var i = 0 ; i < changed_strands.length ; i++) {
			if (changed_strands[i]) { this.show(i,this.config.transitionTime); };
		};
	  } catch (error) {
		  console.log("MMM-LCD-Backlight error in setting lights for ", payload.sender) ;
	  }
	} else if (notification === 'CONFIG') {
		this.config = payload ;
		this.create_strands(this.config.nStrings,this.config.nPixels);
		for (var s = 0 ; s < this.config.nStrings ; s++) {
			this.set_permanent(s+1,this.config.defaultColor); 
			this.show(s+1,0) ;
		}
	} 
  }
  
});
