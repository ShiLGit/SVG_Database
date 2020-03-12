'use strict'

// C library API
const ffi = require('ffi-napi');

// Express App (Routes)
const express = require("express");
const app     = express();
const path    = require("path");
const fileUpload = require('express-fileupload');

app.use(express.json());
app.use(fileUpload());
app.use(express.static(path.join(__dirname+'/uploads')));

// Minimization
const fs = require('fs');
const JavaScriptObfuscator = require('javascript-obfuscator');

// Important, pass in port as in `npm run dev 1234`, do not change
const portNum = process.argv[2];


const svgParse = ffi.Library('./libsvgparse.so', {
  'fileNameToJSON':['string', ['string']],
  'fileNameToDetailedJSON':['string', ['string']],
  'getAttribute':['string', ['string', 'int', 'int']]
});

//respond to req for all images 
app.get('/all',function(req,res){

  fs.readdir(path.join(__dirname+'/uploads'), function (err, files) {
    let allSvgs = {files: []};

    //handling error
    if (err) {
        return console.log('Unable to scan directory: ' + err);
    } 
    //listing all files using forEach
    files.forEach(function (file) {
      const str = svgParse.fileNameToJSON("uploads/" + file)
      //run this block if the SVG is valid 
      if(!(!str || str=="{}")){
        let stats = fs.statSync(path.join(__dirname + '/uploads/' + file));
        const ele = JSON.parse(str);
        ele.fileName = file;
        ele.fileSize = stats.size + " bytes";
        allSvgs.files.push(ele);
      }else{
        console.log("Invalid SVG - will not be sent to index.js");
      }
    
    });
    
    res.send(allSvgs);
  });

});

// Send HTML at root, do not change
app.get('/',function(req,res){
  res.sendFile(path.join(__dirname+'/public/index.html'));
});

// Send Style, do not change
app.get('/style.css',function(req,res){
  //Feel free to change the contents of style.css to prettify your Web app
  res.sendFile(path.join(__dirname+'/public/style.css'));
});

// Send obfuscated JS, do not change
app.get('/index.js',function(req,res){
  fs.readFile(path.join(__dirname+'/public/index.js'), 'utf8', function(err, contents) {
    const minimizedContents = JavaScriptObfuscator.obfuscate(contents, {compact: true, controlFlowFlattening: true});
    res.contentType('application/javascript');
    res.send(minimizedContents._obfuscatedCode);
  });
});

//Respond to POST requests that upload files to uploads/ directory
app.post('/upload', function(req, res) {
  if(!req.files) {
    return res.status(400).send('No files were uploaded.');
  }
 
  let uploadFile = req.files.uploadFile;
 
  // Use the mv() method to place the file somewhere on your server
  uploadFile.mv('uploads/' + uploadFile.name, function(err) {
    if(err) {
      return res.status(500).send(err);
    }

    res.redirect('/');
  });
});

//Respond to GET requests for files in the uploads/ directory
app.get('/uploads/:name', function(req , res)
{
  let json =svgParse.fileNameToDetailedJSON("uploads/" + req.params.name);
  res.send(JSON.parse(json));

  /*wat does this do (old code)
  fs.stat('uploads/' + req.params.name, function(err, stat) {
    if(err == null) {
      //res.sendFile(path.join(__dirname+'/uploads/' + req.params.name));
      
    
    } else {
      console.log('Error in file downloading route: '+err);
      res.send('');
    }
  });*/
});
app.post('/attributes', function(req, res)
{
  const reqData = req.body;
  let str = "[]";
  console.log('DATA:', req.body);

  console.log(str);
  if(reqData.type == 'rect'){
    str = svgParse.getAttribute("uploads/" + reqData.filename, 2, reqData.num-1);
  }else if(reqData.type == 'circ'){
    str = svgParse.getAttribute("uploads/" + reqData.filename, 1, reqData.num-1);
  }else if(reqData.type == 'path'){
    str = svgParse.getAttribute("uploads/" + reqData.filename, 3, reqData.num-1);
  }else if(reqData.type == 'group'){
    str = svgParse.getAttribute("uploads/" + reqData.filename, 4, reqData.num-1);
  }
  console.log(str);
  const json = `{"attr": ${str}}`
  console.log(json);
  res.send(json);
  /*wat does this do (old code)
  fs.stat('uploads/' + req.params.name, function(err, stat) {
    if(err == null) {
      //res.sendFile(path.join(__dirname+'/uploads/' + req.params.name));
      
    
    } else {
      console.log('Error in file downloading route: '+err);
      res.send('');
    }
  });*/
});


//******************** Your code goes here ******************** 

//Sample endpoint
app.get('/someendpoint', function(req , res){
  let retStr = req.query.name1 + " " + req.query.name2;
  res.send({
    foo: retStr
  });
});

app.listen(portNum);
console.log('Running app at localhost: ' + portNum);