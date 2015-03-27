function buildmenu() {
	var menu = "";
	menu += "<nav class=\"navbar navbar-inverse navbar-fixed-top\">";
	menu +=   "<div class=\"container\">";
	menu +=     "<div class=\"navbar-header\">";
//	menu +=       "<button type=\"button\" class=\"navbar-toggle collapsed\" data-toggle=\"collapse\" data-target=\"#navbar\" aria-expanded=\"false\" aria-controls=\"navbar\">";
//	menu +=         "<span class=\"sr-only\">Toggle navigation</span>";
//	menu +=         "<span class=\"icon-bar\"></span>";
//	menu +=         "<span class=\"icon-bar\"></span>";
//	menu +=         "<span class=\"icon-bar\"></span>";
//	menu +=       "</button>";
	menu +=       "<a class=\"navbar-brand\" href=\"#\">SAT>IP</a>";
	menu +=     "</div>";
	menu +=     "<div>";
	menu +=       "<ul class=\"nav navbar-nav\">";
	menu +=         "<li id=\"index\"    class=\"\"><a href=\"index.html\">Home</a></li>";
	menu +=         "<li id=\"log\"      class=\"\"><a href=\"log.html\">Log</a></li>";
	menu +=         "<li id=\"frontend\" class=\"\"><a href=\"frontend.html\">Frontend</a></li>";
	menu +=         "<li id=\"config\"   class=\"\"><a href=\"config.html\">Configure</a></li>";
	menu +=         "<li id=\"contact\"  class=\"\"><a href=\"#contact\">Contact</a></li>";
	menu +=         "<li id=\"about\"    class=\"\"><a href=\"#about\">About</a></li>";
	menu +=       "</ul>";
	menu +=       "<ul class=\"nav navbar-nav navbar-right\">";
	menu +=         "<li id=\"stop\" class=\"\"><a href=\"STOP\"><span class=\"glyphicon glyphicon-off\"></span>Stop</a></li>";
	menu +=       "</ul>";
	menu +=     "</div>";
	menu +=   "</div>";
	menu += "</nav><!-- nav end -->";
	return menu;
}

function setMenuItemActive(param) {
	var obj = document.getElementById(param);
	obj.className = "active";
}