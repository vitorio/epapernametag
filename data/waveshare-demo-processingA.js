// Extracted from scripts.h, Waveshare Team, v1.0.0, 23-January-2018
// + patched to restrict to 4.2" (nametag size) displays

var srcBox,srcImg,dstImg;
var epdArr,epdInd,palArr;
var curPal;
function getElm(n){return document.getElementById(n);}
function setInn(n,i){ document.getElementById(n).innerHTML=i;}

epdInd=1;

palArr=[[[0,0,0],[255,255,255]], [[0,0,0],[255,255,255],[127,0,0]], [[0,0,0],[255,255,255],[220,180,0]]];

epdArr=[[400,300,0], [400,300,1], [400,300,2]];
