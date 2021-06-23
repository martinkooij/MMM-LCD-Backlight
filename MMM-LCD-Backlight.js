/* Magic Mirror
 * Module: MagicMirror_LCD Backlights
 *
 * Martin Kooij (2021)
 *
 * MIT Licensed.
 */
 
 Module.register('MMM-LCD-Backlight', {
	defaults: {
        nStrings: 2,  		// There are two strings
		nPixels: [30,30],		// Each string has 30 leds
        defaultColor: {r: 255, g: 167, b: 87 },   // default = warm white 2700K
        colorCommands: [], // empty list of colorcommands
		luxLevels:[70,450,700],  // 25% upto 70 lux, to 50% at 70, to 75% at 450, to 100% at 700 or higher
		transitionTime: 5000 
	},
	
	start: function() {
		Log.info('Starting module: ' + this.name);
		this.sendSocketNotification('CONFIG', this.config) ; // send module's config to the node_handler, who is doing the job. 
	},
		
   // there is no rendering wait for modules to give commands
   
   notificationReceived: function(notification, payload, sender) {
	   if (notification === 'SET_LCD_BACKLIGHT') {
	   payload.sender = sender.name ;
//	   console.log("AND THIS IS RECEIVED:", this.name, payload);	 
	   this.sendSocketNotification('SET_LIGHTS',payload) ;
	   };
   },
   
});