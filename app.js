'use strict'

// C library API
const ffi = require('ffi-napi');

// Express App (Routes)
const express = require("express");
const app     = express();
const path    = require("path");
const fileUpload = require('express-fileupload');
const config = require('./config.js');
const mysql = require('mysql2/promise');

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
  'getAttribute':['string', ['string', 'int', 'int']],
  'setAttrFile': ['int', ['string', 'string', 'int', 'int']],
  'setTDFile':['int', ['string', 'string']],
  'makeEmpty': ['int', ['string']],
  'addRectToFile': ['int', ['string', 'string']],
  'addCircToFile': ['int', ['string', 'string']],
  'scaleRect': ['int', ['string', 'float']],
  'scaleCirc': ['int', ['string', 'float']]
});


//see if db connection is valid
app.post('/db', async function(req, res, next){
  const loginData = req.body;
  let connection;
  let err = null;

  try{
    connection = await mysql.createConnection({
      host     : loginData.host,
      user     : loginData.user,
      password : loginData.password,
      database : loginData.database
    });
    
  }catch(e){
    err = e;
  }finally{
    if (connection && connection.end) connection.end();

    if(err){
      res.send({error: err});
    }else{
      res.send({success: "Database connection successful."});
    }
  }
});
app.get('/insertdl/:file', function(req, res){
  console.log("INSERTDL CALLED???");
  res.send({success: "lol jk"});
  
})
//respond to saveall files in file log
app.post('/saveall', async function(req, res, next){
  const loginData = req.body;
  console.log("LoginData = ", loginData);
  let connection;
  let err = null;
  let warnings = "";
  const allFiles = [];

  fs.readdir(path.join(__dirname + '/uploads'), async function(err,files){  
    if(err){
     console.log(err);
     return res.send({error: err});
    }
  
    //parse filedata
    files.forEach(function(file){
      console.log(file);

      //get file data 
      const dataStats = svgParse.fileNameToJSON('uploads/' + file);
      const dataDets = svgParse.fileNameToDetailedJSON('uploads/' + file);
      const stats = fs.statSync(path.join(__dirname + '/uploads/' + file));

      if(!dataStats || dataStats === "{}"){
        warnings += `\n${file} is not a valid SVG and was not saved.`;
        return; //CONTINUES FOREACH, DOES NOT BREAK THE WHOLE THING
      }
      const dataStatsObj = JSON.parse(dataStats);
      const dataDetsObj = JSON.parse(dataDets);
      const fileData = {...dataStatsObj, size: stats.size, title: dataDetsObj.title?dataDetsObj.title:null, name: file, desc: dataDetsObj.desc?dataDetsObj.desc:null};

      allFiles.push(fileData);
    });

    //db access attempt
    try{
      connection = await mysql.createConnection({
        host     : loginData.host,
        user     : loginData.user,
        password : loginData.password,
        database : loginData.database
      });
      console.log("Login successful");

      //For each file: check if DNE in database, enter info to FILE table if so
      for(let i = 0; i < allFiles.length; i++){
        let file = allFiles[i];
        const [rows, fields] = await connection.execute(`SELECT * FROM FILE WHERE FILE.file_name='${file.name}'`);
        
        //entry of this filename DNE, insert
        if(rows.length ===0){
          console.log('...saving file...')
          await connection.execute(`INSERT INTO FILE(file_name, file_title, file_description, n_rect, n_circ, n_path, n_group, creation_time, file_size)
                              VALUES('${file.name}', '${file.title}', '${file.desc}', ${file.numRect}, ${file.numCirc}, ${file.numPaths}, ${file.numGroups}, ${Date.now()}, ${file.size})`)
          }
      }
    }catch(e){
      err = e;
    }finally{
      if (connection && connection.end) connection.end();
      if(err){
        console.log(err);
        res.send({error: err});
      }else{
        res.send({success: "Database connection successful."});
      }
    }
  

  });

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
  console.log(uploadFile);
  if(uploadFile.mimetype !== 'image/svg+xml'){
   // res.send(JSON.stringify({error: "File is not a valid SVG."}));
    return;
  }else{
  //  res.send(JSON.stringify({success: "CONGRATULATIONS!!!!! YOU SENT AN SVG!!!!!"}));
  }

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
app.post('/updateattribute', function(req, res){
  const reqData = req.body;
  console.log(JSON.stringify(reqData.attr));
  let flag = svgParse.setAttrFile('uploads/'+reqData.filename, JSON.stringify(reqData.attr), enumerate(reqData.type), reqData.num-1);
  console.log(flag, req.body);

  if(flag == -1){
    res.send(JSON.stringify({error: "Invalid attribute. Change was not saved."}));
  }else if( flag == 0){
    res.send(JSON.stringify({error: reqData.filename + " could not validate against svg.xsd."}));
  }else{
    res.send(JSON.stringify({success: "SVG changed successfully."}));
  }
});

app.post('/updatetd', function(req, res){
  const reqData = req.body;
  console.log(JSON.stringify(reqData));
  let flag =svgParse.setTDFile(reqData.filename, JSON.stringify(reqData));

  if(flag){
    res.send(JSON.stringify({success: "SVG changed successfully."}));
  }else{
    res.send(JSON.stringify({error: 'Could not validate SVG.'}));
  }
})
function enumerate(str){

  if(str == 'circ'){
    return 1;
  }else if (str == 'rect'){
    return 2;
  }else if (str == 'path'){
    return 3;
  }else if (str == 'group'){
    return 4;
  }
  return 0;
}
app.post('/create', function(req,res){
    const name = req.body.name;
    fs.readdir(path.join(__dirname+'/uploads'), function (err, files) {
        //listing all files using forEach
        let dupe = false;
        files.forEach(function (file) {
            if(file.toString() == name){
                dupe = true;
                console.log("Duplicate file");
            }
        });

        if(dupe){
            return res.send({error: "File already exists"});
        }
        let flag = svgParse.makeEmpty("uploads/" + name);
        if(!flag){
            return res.send({error: "Could not make file."});
        }
        return res.send({success: "Successfully created " + name +"."});
      });
});
app.post('/attributes', function(req, res)
{
  const reqData = req.body;
  let str = "[]";
  str = svgParse.getAttribute("uploads/" + reqData.filename, enumerate(reqData.type), reqData.num-1);

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
app.post('/scalerect/:file', function(req,res){
  console.log('fnma:', req.params.file);
  console.log(req.body);
  let flag = svgParse.scaleRect('uploads/'+req.params.file, req.body.factor);
  if(!flag){
    return res.send("ERROR: Unable to scale rectangles.");
  }else{
    return res.send("Rects scaled successfully.");
  }
});
app.post('/scalecirc/:file', function(req,res){
  console.log('fnma:', req.params.file);
  console.log(req.body);
  let flag = svgParse.scaleCirc('uploads/'+req.params.file, req.body.factor);
  if(!flag){
    return res.send("ERROR: Unable to scale circs.");
  }else{
    return res.send("Circs scaled successfully.");
  }
});
app.post('/addshape/:file', function(req, res){
  const arg = req.body;
  if(!arg){
    return res.send('ERROR!');
  }

  if(arg.rect){
    console.log('adding r');
    let flag = svgParse.addRectToFile("uploads/" + req.params.file, JSON.stringify(arg));
    console.log(flag)

    if(flag == 0){
      return res.send("ERROR: Could not validate file after adding component (rect)");
    }else{
      return res.send("Rect saved successfully.");
    }
  }
  if(arg.circ){
    let flag = svgParse.addCircToFile("uploads/" + req.params.file, JSON.stringify(arg));
    console.log(flag)

    if(flag == 0){
      return res.send("ERROR: Could not validate file after adding component (circ)");
    }else{
      return res.send("Circ saved successfully.");
    }
  }
  res.send(null);
})


app.listen(portNum);
console.log('Running app at localhost: ' + portNum);
