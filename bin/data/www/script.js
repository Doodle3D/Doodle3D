var oopsTimer;
var dragging;
var path;
var svg;

$(window).ready(function() {

	path = document.getElementById('path');
	svg = document.getElementById('svg');

	svg.addEventListener('mousedown',onMouseDown,false);
	svg.addEventListener('mousemove',onMouseMove,false);
	svg.addEventListener('mouseup',onMouseUp,false);
	svg.addEventListener('touchstart',onTouchDown,false);
	svg.addEventListener('touchmove',onTouchMove,false);
	btnNew.addEventListener('mousedown',clear,false);
	btnNew.addEventListener('touchstart',clear,false);
	btnOops.addEventListener('touchstart',startOops,false);
	btnOops.addEventListener('touchend',stopOops,false);
	btnOops.addEventListener('mousedown',startOops,false);
	btnOops.addEventListener('mouseup',stopOops,false);
	btnPrint.addEventListener('mousedown',print,false);
	btnPrint.addEventListener('touchstart',print,false);
	document.body.addEventListener('touchmove',prevent,false);
});

function prevent(e) {
	e.preventDefault();
}

function clear() {
	path.attributes.d.nodeValue = "M0 0";
}

function startOops() {
	oopsTimer = setInterval("oops()",1000/30);
}

function stopOops() {
	clearInterval(oopsTimer);
}

function oops() {
	str = path.attributes.d.nodeValue;
	n = str.lastIndexOf(" L");
	path.attributes.d.nodeValue = str.substr(0,n);	
}

function moveTo(x,y) {
	if (path.attributes.d.nodeValue=="M0 0") {
		path.attributes.d.nodeValue = "M" + x + " " + y;
	} else {
		path.attributes.d.nodeValue += " M" + x + " " + y;
	}
}

function lineTo(x,y) {
	path.attributes.d.nodeValue += " L" + x + " " + y;
}

function onTouchDown(e) {
	var x = e.touches[0].pageX - e.touches[0].target.offsetLeft;
	var y = e.touches[0].pageY - e.touches[0].target.offsetTop;
	moveTo(x,y);	
}

function onTouchMove(e) {
	e.preventDefault();
	var x = e.touches[0].pageX - e.touches[0].target.offsetLeft;
	var y = e.touches[0].pageY - e.touches[0].target.offsetTop;
	lineTo(x,y);
}

function onMouseDown(e) {
	dragging = true;
	moveTo(e.offsetX,e.offsetY);
}

function onMouseMove(e) {
	if (!dragging) return;
	lineTo(e.offsetX,e.offsetY);
}

function onMouseUp(e) {
	dragging = false;
}

function print(e) {

	output = path.attributes.d.nodeValue;
	console.log(output);
	
	output = output.split("M").join("\n");
	output = output.split(" L").join("_");
	output = output.split(" ").join(",");
	output = output.split("_").join(" ");

	output = "\nBEGIN\n" + output + "\n\nEND\n";

	$.post("/doodle3d.of", { data:output }, function(data) {
	 	btnPrint.disabled = false;
	 	//alert(data);
	});
}
