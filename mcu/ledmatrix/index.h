#pragma once

const char index_html[] = 
"<!doctype html>\n \
<html>\n \
\n \
<head>\n \
<title>LED Matrix</title>\n \
<meta charset='utf-8'>\n \
<style>\n \
.button { font-size: 30px; width:100%; height:200px }\n \
.cell { width:25% }\n \
</style>\n \
</head>\n \
\n \
<body>\n \
<script type='text/javascript'>\n \
function postURL(url)\n \
{\n \
	const request = new XMLHttpRequest();\n \
	request.open('POST', url, true);\n \
	request.send();\n \
}\n \
</script>\n \
<table style='width:100%'>\n \
<tr>\n \
<td class='cell'><button style='background-color:lightgray' class='button' onclick='postURL(\"/eff/0\");'>BLACK</button></td>\n \
<td class='cell'><button style='background-color:lightgray' class='button' onclick='postURL(\"/eff/1\");'>SQUARE</button></td>\n \
<td class='cell'><button style='background-color:lightgray' class='button' onclick='postURL(\"/eff/2\");'>CIRCLE</button></td>\n \
<td class='cell'><button style='background-color:lightgray' class='button' onclick='postURL(\"/eff/3\");'>PLASMA</button></td>\n \
</tr>\n \
<tr>\n \
<td class='cell'><button style='background-color:lightgray' class='button' onclick='postURL(\"/eff/4\");'>FIRE</button></td>\n \
<td class='cell'><button style='background-color:lightgray' class='button' onclick='postURL(\"/eff/5\");'>BLOB</button></td>\n \
<td class='cell'><button style='background-color:lightgray' class='button' onclick='postURL(\"/eff/6\");'>PIXELS</button></td>\n \
<td class='cell'><button style='background-color:lightgray' class='button' onclick='postURL(\"/eff/7\");'>VOXEL</button></td>\n \
</tr>\n \
<tr>\n \
<td class='cell'><button style='background-color:#FFD700' class='button' onclick='postURL(\"/pal/0\");'>RAINBOW</button></td>\n \
<td class='cell'><button style='background-color:#FFA500' class='button' onclick='postURL(\"/pal/1\");'>FIRE</button></td>\n \
<td class='cell'><button style='background-color:#00FFFF' class='button' onclick='postURL(\"/pal/2\");'>TEMP</button></td>\n \
<td class='cell'><button style='background-color:#F5DEB3' class='button' onclick='postURL(\"/pal/3\");'>EARTH</button></td>\n \
</tr>\n \
<tr>\n \
<td class='cell'><button style='background-color:#FFFF00' class='button' onclick='postURL(\"/pal/4\");'>YELLOW</button></td>\n \
<td class='cell'><button style='background-color:#00FF7F' class='button' onclick='postURL(\"/pal/5\");'>CYAN</button></td>\n \
<td class='cell'><button style='background-color:#FF00FF' class='button' onclick='postURL(\"/pal/6\");'>PURPLE</button></td>\n \
<td class='cell'><button style='background-color:#00FFFF' class='button' onclick='postURL(\"/pal/7\");'>CYAN2</button></td>\n \
</tr>\n \
<tr>\n \
<td class='cell'><button style='background-color:white' class='button' onclick='postURL(\"/pal/8\");'>GRAD</button></td>\n \
<td class='cell'><button style='background-color:white' class='button' onclick='postURL(\"/pal/9\");'>MIDDLE</button></td>\n \
<td class='cell'><button style='background-color:white' class='button' onclick='postURL(\"/pal/10\");'>HIGH</button></td>\n \
<td class='cell'><button style='background-color:white' class='button' onclick='postURL(\"/pal/11\");'>RING</button></td>\n \
</tr>\n \
</table>\n \
</body>\n \
\n \
</html>\n";
