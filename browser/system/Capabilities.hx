package browser.system;


import browser.Html5Dom;
import browser.Lib;


class Capabilities {
	
	
	public static var hasAccessibility = false;
	public static var pixelAspectRatio (get_pixelAspectRatio, null):Float;
	public static var screenDPI (get_screenDPI, null):Float;
	public static var screenResolutionX (get_screenResolutionX, null):Float;
	public static var screenResolutionY (get_screenResolutionY, null):Float;
	public static var language (get_language, null):String;
	
	
	
	
	// Getters & Setters
	
	
	
	
	private static function get_pixelAspectRatio ():Float { return 1; }
	private static function get_screenDPI ():Float {
		
		if (screenDPI > 0) return screenDPI;
		
		//little trick of measuring the width of a 1 inch div
		//but sadly most browsers/OSs still return wrong result...
		var body = Lib.document.getElementsByTagName("body")[0];
		var testDiv:HTMLElement = cast Lib.document.createElement ("div");
		testDiv.style.width = testDiv.style.height = "1in";
		testDiv.style.padding = testDiv.style.margin = "0px";
		testDiv.style.position = "absolute";
		testDiv.style.top = "-100%";
		body.appendChild(testDiv);
		screenDPI = testDiv.offsetWidth;
		body.removeChild(testDiv);
		
		return screenDPI;
		
	}
	
	
	private static function get_screenResolutionX ():Float { 
		
		return Std.parseFloat (Lib.window.screen.width);
		
	}
	
	
	private static function get_screenResolutionY ():Float {
		
		return Std.parseFloat (Lib.window.screen.height);
		
	}
	
	
	private static function get_language ():String { return untyped navigator.language; }
	
	
}