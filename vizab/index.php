<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01//EN" "http://www.w3.org/TR/html4/strict.dtd">
<html>
<head>
<title>BA3.0</title>
<meta name="viewport" content="initial-scale=1, width=650"/>
<script type="text/javascript" src="index.js"></script>
<script type="text/javascript" src="vizab.js"></script>
<script type="text/javascript">

function showgraph(airid)
{
document.getElementById("graph1").src="/intern/munin/localdomain/localhost.localdomain-airid" + airid + "-day.png"
document.getElementById("graph2").src="/intern/munin/localdomain/localhost.localdomain-airid" + airid + "-week.png"
document.getElementById("graph3").src="/intern/munin/localdomain/localhost.localdomain-airid" + airid + "-month.png"
document.getElementById("graph4").src="/intern/munin/localdomain/localhost.localdomain-airid" + airid + "-year.png"
}

</script>
</head>

<body style="font-family:helvetica;font-size:12px;">

<img style="position:absolute;top:0px;left:0px" src="zuhause.png">

<img style="position:absolute;top:80px;left:400px" src="about:blank" id="WZSofa">
<img style="position:absolute;top:230px;left:300px" src="about:blank" id="WZKugeln">
<img style="position:absolute;top:210px;left:410px" src="about:blank" id="WZHeizung">
<input type="text" style="position:absolute;top:100px;left:400px;width:45px" id="WZSofaText">

<input onclick="showgraph('0001');" style="border:0px;background-color:transparent;position:absolute;top:100px;left:170px;width:60px;" unit="�C" type="text" value="x" name="0001" id="airid0001">
<input onclick="showgraph('0002');" style="border:0px;background-color:transparent;position:absolute;top:330px;left:70px;width:60px;" unit="�C" type="text" value="x" name="0002" id="airid0002">
<input onclick="showgraph('0003');" style="border:0px;background-color:transparent;position:absolute;top:380px;left:400px;width:60px;" unit="�C" type="text" value="x" name="0003" id="airid0003">
<input onclick="showgraph('0004');" style="border:0px;background-color:transparent;position:absolute;top:210px;left:470px;width:60px;" unit="�C" type="text" value="x" name="0004" id="airid0004">
<input onclick="showgraph('0005');" style="border:0px;background-color:transparent;position:absolute;top:470px;left:400px;width:60px;" unit="�C" type="text" value="x" name="0005" id="airid0005">
<input onclick="showgraph('0101');" style="border:0px;background-color:transparent;position:absolute;top:30px;left:540px;width:60px;" unit="cm" type="text" value="x" name="0101" id="airid0101">
<input onclick="showgraph('0102RH');" style="border:0px;background-color:transparent;position:absolute;top:50px;left:540px;width:60px;" unit="%" type="text" value="x" name="0102RH" id="airid0102RH">
<input onclick="showgraph('0102P');" style="border:0px;background-color:transparent;position:absolute;top:70px;left:540px;width:60px;" unit="hPa" type="text" value="x" name="0102P" id="airid0102P">
<input onclick="showgraph('0102T');" style="border:0px;background-color:transparent;position:absolute;top:90px;left:540px;width:60px;" unit="�C" type="text" value="x" name="0102T" id="airid0102T">
<input onclick="showgraph('0102D');" style="border:0px;background-color:transparent;position:absolute;top:110px;left:540px;width:60px;" unit="�C" type="text" value="x" name="0102D" id="airid0102D">

<span style="position:absolute;top:34px;left:460px">Wasserstand</span>
<span style="position:absolute;top:54px;left:460px">Luftfeuchte</span>
<span style="position:absolute;top:74px;left:460px">Luftdruck</span>
<span style="position:absolute;top:94px;left:460px">Temperatur</span>
<span style="position:absolute;top:114px;left:460px">Taupunkt</span>

<img style="position:absolute;top:700px;left:20px;width:495px" src="about:blank" id="graph1">
<img style="position:absolute;top:970px;left:20px;width:495px" src="about:blank" id="graph2">
<img style="position:absolute;top:1240px;left:20px;width:495px" src="about:blank" id="graph3">
<img style="position:absolute;top:1510px;left:20px;width:495px" src="about:blank" id="graph4">

<a style="position:absolute;top:170px;left:500px" href="/icinga/">Icinga</a>
<a style="position:absolute;top:190px;left:500px" href="/intern/munin/localdomain/localhost.localdomain.html#Sensoren">Munin</a>

</body>
</html>