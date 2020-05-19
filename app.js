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


//parse data for FILE record given fname
function parsedata_FILE(fname){
  const dataStats = svgParse.fileNameToJSON('uploads/' + fname);
  const dataDets = svgParse.fileNameToDetailedJSON('uploads/' +  fname);
  const stats = fs.statSync(path.join(__dirname + '/uploads/' +  fname));
  
  if(!dataStats || dataStats === "{}"){
    return {error: "Error parsing file data for " + fname}; //CONTINUES FOREACH, DOES NOT BREAK THE WHOLE THING
  }

  const dataStatsObj = JSON.parse(dataStats);
  const dataDetsObj = JSON.parse(dataDets);
  const fileData = {...dataStatsObj, size: stats.size, title: dataDetsObj.title?dataDetsObj.title:null, name:  fname, desc: dataDetsObj.desc?dataDetsObj.desc:null};
  
  console.log('fdata: ', fileData);
  return {success: fileData};
}
function formatted_Date(){
  const curDate = new Date();
  return curDate.getDay() + "/" + curDate.getMonth()+1 + "/" + curDate.getFullYear();
}
function formatted_Datetime(){
  let r =new Date().toISOString().slice(0, 19).replace('T', ' ');
  console.log('formdate = ',  r);
  return r;
}

//see if db connection is valid; init tables if dne
app.post('/db', async function(req, res, next){
  const loginData = req.body;
  let connection;
  let err = null;
  let success = "Database connection successful.";
  try{
    connection = await mysql.createConnection({
      host     : loginData.host,
      user     : loginData.user,
      password : loginData.password,
      database : loginData.database
    });
  
  //keeps track of existing tables
  const existingTables = [];
  //check if tables exist
  const [rows, fields] = await connection.execute("SHOW TABLES");
  for(let i = 0; i < rows.length; i++){
    let obj = rows[i];
    console.log(i,obj, obj[(Object.keys(obj)[0])]);
    existingTables.push(obj[(Object.keys(obj)[0])]);
  }
  console.log('existing tables', existingTables);
  //init tables
  if(!existingTables.includes('FILE') || !existingTables.includes('IMG_CHANGE') || !existingTables.includes('DOWNLOADS')){
    //drop all tables if they exist
    if(existingTables.includes('FILE'))
      await connection.execute("DROP TABLE FILE");
    if(existingTables.includes('IMG_CHANGE'))
      await connection.execute("DROP TABLE IMG_CHANGE");
    if(existingTables.includes('DOWNLOAD'))
      await connection.execute("DROP TABLE DOWNLOAD");  
    
    //init all tables
    await connection.execute(`CREATE TABLE FILE(svg_id INT NOT NULL AUTO_INCREMENT, 
                                                file_name VARCHAR(60) NOT NULL, 
                                                file_title VARCHAR(256), 
                                                file_description VARCHAR(256), 
                                                n_rect INT NOT NULL, 
                                                n_circ INT NOT NULL, 
                                                n_group INT NOT NULL, 
                                                creation_time DATETIME NOT NULL, 
                                                file_size INT NOT NULL, 
                                                PRIMARY KEY(svg_id))`);
    await connection.execute(`CREATE TABLE IMG_CHANGE(change_id INT AUTO_INCREMENT,
                                                      change_type VARCHAR(256) NOT NULL,
                                                      change_summary VARCHAR(256) NOT NULL,
                                                      change_time DATETIME NOT NULL,
                                                      svg_id INT NOT NULL,
                                                      FOREIGN KEY(svg_id) REFERENCES FILE(svg_id) ON DELETE CASCADE,
                                                      PRIMARY KEY(change_id))`);
    await connection.execute(`CREATE TABLE DOWNLOAD(download_id INT AUTO_INCREMENT,
                                                    d_descr VARCHAR(256),
                                                    svg_id INT NOT NULL,
                                                    FOREIGN KEY(svg_id) REFERENCES FILE(svg_id) ON DELETE CASCADE,
                                                    PRIMARY KEY(download_id))`);
  }

  }catch(e){
    console.log(e);
    err = e;
  }finally{
    if (connection && connection.end) connection.end();

    if(err){
      res.send({error: err});
    }else{
      res.send({success});
    }
  }
});

app.post('/insertdl/:file', async function(req, res, next){
  console.log("INSERTDL CALLED???");

  const connectionData = req.body;
  console.log(connectionData);
  let connection;
  let successful = false;
  const fname = req.params.file;

  try{
    connection = await mysql.createConnection({
      host     : connectionData.host,
      user     : connectionData.user,
      password : connectionData.password,
      database : connectionData.database
    });
    console.log("insertdl: dbconnection successful");

    //INSERT FILE INTO DB IF NONEXISTING
    const [rows, fields] = await connection.execute(`SELECT * FROM FILE WHERE FILE.file_name='${fname}'`);

    if(rows.length === 0){
      //parse, insert FILE record if DNE 
      const data = parsedata_FILE(fname);
      if(data.success){
        const file= data.success;
        await connection.execute(`INSERT INTO FILE(file_name, file_title, file_description, n_rect, n_circ, n_path, n_group, creation_time, file_size)
                                  VALUES('${file.name}', '${file.title}', '${file.desc}', ${file.numRect}, ${file.numCirc}, ${file.numPaths}, ${file.numGroups}, '${formatted_Datetime()}', ${file.size})`)
      }else if (data.error){
        throw data.error;
      }
    }

    //INSERT DOWNLOAD RECORD
    await connection.execute(`INSERT INTO DOWNLOAD(d_descr, svg_id) VALUES('${formatted_Date()}', ${rows[0].svg_id})`)   

  }catch(e){
    console.log(e);
    res.send({error: e});

  }finally{
    if(connection && connection.end) connection.end();
  }
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
      const fileData = parsedata_FILE(file);
      if(fileData.success){
        allFiles.push(fileData.success);
      }else if (fileData.error){
        warnings += fileData.error;
      }
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
          await connection.execute(`INSERT INTO FILE(file_name, file_title, file_description, n_rect, n_circ, n_path, n_group, creation_time, file_size)
                              VALUES('${file.name}', '${file.title}', '${file.desc}', ${file.numRect}, ${file.numCirc}, ${file.numPaths}, ${file.numGroups}, '${formatted_Datetime()}', ${file.size})`);
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
        res.send({success: "Database connection successful." + warnings});
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

//update title/desc
app.post('/updatetd', async function(req, res){
  const reqData = req.body.update;
  console.log(req.body.loginData);
  const loginData = req.body.loginData;
  let flag =svgParse.setTDFile(reqData.filename, JSON.stringify(reqData));

  if(!flag){
    return res.status(400).send(JSON.stringify({error: 'Could not validate SVG.'}));
  }

  //db connection 
  let error = null;
  let connection;
  try{
    connection = await mysql.createConnection({
        host     : loginData.host,
        user     : loginData.user,
        password : loginData.password,
        database : loginData.database
    });
    const filename = reqData.filename.replace("uploads/", "");
    console.log(filename);

    const [rows, fields] = await connection.execute(`SELECT svg_id FROM FILE WHERE FILE.file_name='${filename}'`);

    //insert FILE record if DNE
    if(rows.length === 0){
      const data = parsedata_FILE(filename);
      if(data.success){
        const file= data.success;
        console.log(`INSERT INTO FILE(file_name, file_title, file_description, n_rect, n_circ, n_path, n_group, creation_time, file_size)
        VALUES('${file.name}', '${file.title}', '${file.desc}', ${file.numRect}, ${file.numCirc}, ${file.numPaths}, ${file.numGroups}, '${formatted_Datetime()}', ${file.size})`);
        await connection.execute(`INSERT INTO FILE(file_name, file_title, file_description, n_rect, n_circ, n_path, n_group, creation_time, file_size)
                                  VALUES('${file.name}', '${file.title}', '${file.desc}', ${file.numRect}, ${file.numCirc}, ${file.numPaths}, ${file.numGroups}, '${formatted_Datetime()}', ${file.size})`)
      }else{
        throw data.error;
      }
    }
    //insert IMG_CHANGE record
    let changesum = (reqData.title?("TITLE: " + reqData.title +"\n"):"") + (reqData.desc?("DESC: " + reqData.desc): "");
    await connection.execute(`INSERT INTO IMG_CHANGE(change_type, change_summary, change_time, svg_id)
                                              VALUES('EDIT TITLE/DESC', '${changesum}', '${formatted_Datetime()}', ${rows[0].svg_id})`)
   }catch(e){
    error = e;
    console.log(e);
  }finally{
    if(connection && connection.end) connection.end();
  }

  res.send({success: "Updates changed successfully."});
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
app.post('/create', async function(req,res){
    const name = req.body.name;
    let err = null;

    //check if duplicate file name
    await fs.readdir(path.join(__dirname+'/uploads'), function (err, files) {
        //listing all files using forEach
        let dupe = false;
        files.forEach(function (file) {
            if(file.toString() == name){
                dupe = true;
            }
        });

        if(dupe){
          console.log("Error: duplicate file.")
          err = {error: "File already exists"};
        }
        let flag = svgParse.makeEmpty("uploads/" + name);
        if(!flag){
            err = {error: "Could not make file."};
        }
      });

      if(err){
        return res.status(400).send(err);
      }

      //make db connection, update FILE table 
      const loginData = req.body.loginData;
      try{
        let connection = await mysql.createConnection({
          host     : loginData.host,
          user     : loginData.user,
          password : loginData.password,
          database : loginData.database
        });
        const fileData = parsedata_FILE(name);
        if(fileData.error){
          throw fileData.error;
        }
        const file = fileData.success;
        await connection.execute(`INSERT INTO FILE(file_name, file_title, file_description, n_rect, n_circ, n_path, n_group, creation_time, file_size)
                                              VALUES('${file.name}', '${file.title}', '${file.desc}', ${file.numRect}, ${file.numCirc}, ${file.numPaths}, ${file.numGroups}, '${formatted_Datetime()}', ${file.size})`)

      }catch(e){
        err = e;
        console.log(e);
      }finally{
        if(err) return res.status(400).send(err);
        return res.send({success: "Successfully created " + name +"."});
      }

});

app.post('/attributes', function(req, res)
{
  const reqData = req.body;
  let str = "[]";
  str = svgParse.getAttribute("uploads/" + reqData.filename, enumerate(reqData.type), reqData.num-1);

  const json = `{"attr": ${str}}`
  console.log(json);
  res.send(json);
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

app.post('/addshape/:file', async function(req, res){
  const arg = req.body;
  let error = null;
  let desc = "";
  if(!arg){
    return res.status(400).send({error: "Missing argument"});
  }
  console.log(arg.loginData);

  if(arg.rect){
    let flag = svgParse.addRectToFile("uploads/" + req.params.file, JSON.stringify(arg));
    console.log(flag)

    if(flag == 0){
      return res.status(400).send({error: "Could not validate file after adding component (rect)"});
    }else{
      desc += "Added rectangle: " + JSON.stringify(arg.rect); 
    }
  }
  if(arg.circ){
    let flag = svgParse.addCircToFile("uploads/" + req.params.file, JSON.stringify(arg));
    console.log(flag)

    if(flag == 0){
      return res.status(400).send({error: "Could not validate file after adding component (circ)"});
    }else{
      desc += "\nAdded circle: " + JSON.stringify(arg.circ);
    }
  }
  let connection = null;
  //db entry 
  try{
    let loginData = arg.loginData;
    connection = await mysql.createConnection({
      host     : loginData.host,
      user     : loginData.user,
      password : loginData.password,
      database : loginData.database
    });
    const [rows, fields] = await connection.execute(`SELECT svg_id FROM FILE WHERE FILE.file_name = '${req.params.file}'`)

    //parse, insert FILE record if DNE 
    if(rows.length === 0){
      const data = parsedata_FILE(req.params.file);
      if(data.success){
        const file= data.success;
        await connection.execute(`INSERT INTO FILE(file_name, file_title, file_description, n_rect, n_circ, n_path, n_group, creation_time, file_size)
                                  VALUES('${file.name}', '${file.title}', '${file.desc}', ${file.numRect}, ${file.numCirc}, ${file.numPaths}, ${file.numGroups}, '${formatted_Datetime()}', ${file.size})`)
      }else{
        throw data.error;
      }
    }else{
      await connection.execute(`INSERT INTO IMG_CHANGE(change_type, change_summary, change_time, svg_id)
                                                VALUES('ADD SHAPE', '${desc}', '${formatted_Datetime()}', ${rows[0].svg_id})`);
    }
  }catch(e){
    error = e;
    console.log(e);
  }finally{
    if(connection && connection.end) connection.end();
  }

  if(error){
    return res.status(400).send({error});
  }
  res.send("Changes saved successfully.");
})


app.listen(portNum);
console.log('Running app at localhost: ' + portNum);
