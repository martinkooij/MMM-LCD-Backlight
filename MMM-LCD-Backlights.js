/* Magic Mirror
 * Module: MagicMirror_LCD Backlights
 *
 * Martin Kooij (2021)
 *
 * MIT Licensed.
 */
 
 Module.register('MMM-LCD-Backlights', {
	defaults: {
        nStrings: 2,  		// There are two strings
		nPixels: [30,30],		// Each string has 30 leds
        defaultColor: {r: 255, g: 167, b: 87 },   // default = warm white 2700K
        colorCommands: [], // empty list of colorcommands
		transitionTime: 5000 
	},
	
	start: function() {
		Log.info('Starting module: ' + this.name);
		this.sendSocketNotification('CONFIG', this.config) ; // send module's config to the node_handler, who is doing the job. 
	},
		
   // there is no rendering wait for modules to give commands
   
   notificationReceived: function(notification, payload, sender) {
	   if (notification === 'SET_LCD_BACKLIGHTS') {
	   payload.sender = sender.name ;
//	   console.log("AND THIS IS RECEIVED:", this.name, payload);	 
	   this.sendSocketNotification('SET_LIGHTS',payload) ;
	   };
   },
   
});