// Put all onload AJAX calls here, and event listeners
$(document).ready(function() {
    const fileLog = document.getElementById('file-log');
    let curFile = ""; //file open in file viewer
    let props = {}; //properties of selected omponent

    updateLog();
    toggleAddCirc(false);
    toggleAddRect(false);

    $('#dbfunctions').hide();
    $('#filelog').hide();
    $('#fileeditor').hide();
    $('#dblogin-link').trigger('click');
    //get all files for file laaaaaaaaaaaaaaaaaawg; render on screen
    function updateLog(){
        console.log("updateLog() called");
        $.ajax({
            type: 'get',            //Request type
            dataType: 'json',       //Data type - we will use JSON for almost everything 
            url: '/all',   //The server endpoint we are connecting to
            success: function (allSvgs) {
                //populate table
                let files = allSvgs.files;
                for(let i = 0; i < files.length; i++){
                    //build table row 
                    let row = `<tr>
                        <td>
                        <a href ="${files[i].fileName}" download />
                            <img src = "${files[i].fileName}"  id = "i_${files[i].fileName}" download/>
                        </a>
                        </td>
                        <td ><a href = "${files[i].fileName}" download id = "a_${files[i].fileName}"/>${files[i].fileName} </td>
                        <td>${files[i].fileSize}</td>
                        <td>${files[i].numRect}</td>
                        <td>${files[i].numCirc}</td>
                        <td>${files[i].numPaths}</td>
                        <td>${files[i].numGroups}</td>
                        </tr>`
    
                    fileLog.insertAdjacentHTML('beforeend', row);
                    document.getElementById("i_" + files[i].fileName).onclick =  ()=> insert_dl(files[i].fileName);
                    document.getElementById("a_" + files[i].fileName).onclick =  ()=> insert_dl(files[i].fileName);
                    console.log('insertions for i_' + files[i].fileName + ', a_'+files[i].fileName);
                }
                let options = "";
                for(let i = 0; i < files.length; i++){           
                    options += `<option value="${files[i].fileName}">${files[i].fileName}</option>`;
                }
                if(allSvgs.length >= 5){
                    fileLog.setAttribute("overflow-y", "scroll");
                }
                document.getElementById("svg-name").innerHTML = options;
                 
            },
            fail: function(error) {
                console.log(error); 
            }
        })
    }
//FILEVIEWER STUFFFFFFFF----------------------------------------------------------------------------------------

//choose file to view    
$('#file-select-form').submit(function(e){
        e.preventDefault();
        let svgName =document.getElementById("svg-name").value 
        curFile = svgName;
        $("#file-view-img").attr("src",svgName);

        $.ajax({
                type: 'get',            //Request type
                dataType: 'json',       //Data type - we will use JSON for almost everything 
                url: '/uploads/' + svgName,   //The server endpoint we are connecting to
                success: function(data){
                    renderReport(data);
                    toggleTDForm(true);
                },
                fail: function(err){
                    console.log(err);
                }
        })
    });

    //'Login' to database 
    $('#dblogin-form').submit((e)=>{
        e.preventDefault();
        const loginData = getLoginData();
        if(!loginData){
            return alert("Error: invalid database connection data. Log out and try again.");
        }
        console.log('ldat:', loginData);
        $.ajax({
            type: 'POST',
            contentType: 'application/json',
            data: JSON.stringify(loginData),
            url: '/db',

            success: function(arg){
                console.log(arg)

                if(arg.error){
                    alert("Error: database connection failed.\nLog: " + `'${arg.error.message}'`);
                }else if (arg.success){
                    alert(arg.success);

                    //$('form#dblogin-form :input[type=text]').each(function(){$(this).prop("disabled", true);})
                    $('#dblogin-form').css("display", "none");

                    //replace with logout
                    $('#dblogout').css("display", "block");
                    $('#dblogin-link').trigger('click');

                    //show other controls
                    $('#dbfunctions').show();
                    $('#filelog').show();
                    $('#fileeditor').show();
                }
            },
            fail: function(err){
                alert("ERROR:" + err.e);
            }

        });
    })
    //'Logout' of database
    $('#logout').click(function(){
        $('#dbfunctions').hide();
        $('#filelog').hide();
        $('#fileeditor').hide();
        
        $('#dblogout').css("display", "none");
        $("#dblogin-form").css("display", "block");
        $('#collapsible-fileview').prop("disabled", true);

        $('form#dblogin-form :input[type=text]').each(function(){
            $(this).prop("disabled", false);
            $(this).prop("value", "");  
        })

        alert("Log in to access file editing functionality.");
    })

    $('#storefiles').click(function(e){
        e.preventDefault();
        const loginData = getLoginData();
        if(!loginData){
            return alert("Error: invalid database connection data. Log out and try again.");
        }
        console.log("STOREFILES", loginData);
        $.ajax({
            type: 'POST',
            url: '/saveall',
            contentType: 'application/json',
            data: JSON.stringify(loginData),
            success: function(res){
                if(res.success){
                    alert(res.success);
                }else if(res.error){
                    alert(res.error.message);
                }
            },
            fail: function(err){
                alert("Save failed.");
            } 
        });
    })

    //updates DOWNLOADS table (inserts new row)
    function insert_dl(fileName){
        const loginData = getLoginData();
        if(!loginData){
            return alert("Error: invalid database connection data. Log out and try again.");
        }
        
        $.ajax({
            type: 'POST',
            url:'/insertdl/' + fileName,
            dataType: 'JSON',
            contentType: 'application/JSON',
            data: JSON.stringify(loginData),
            success: function (res){
                alert(res)
            }
        })
    }
    //END OF DB STUFF -------------------------------------



    /*
    Processes data returned from individual SVG request; renders onto file view table
    */
    function renderReport(data){
        let num = 1;
        let rects = data.rects;
        let circs = data.circs;
        let paths = data.paths;
        let groups = data.groups;
        const fview = document.getElementById("file-view");
        let options = "<option value = 'placeholder'></option>";

        let report = 
        `<tr>
            <th>Title</th>
            <th colspan = '2'>Description</th>
        </tr>
        <tr>
            <td id = "fview-title"></td>
            <td colspan = '2' id = "fview-desc"></td>
        </tr>
        <tr>
            <th width = "20%">Component</th>
            <th>Summary</th>
            <th>Other Attributes</th>
        </tr>`;

        rects.forEach((r)=>{
            report += makeFViewEntry({
            name: "Rectangle" + num, 
            summary: `Upper L. Corner: x = ${r.x + r.units}, y = ${r.y + r.units}<br/>
                      Width = ${r.w + r.units}, height = ${r.h + r.units}`,
            numAttr: r.numAttr});

            options += `<option value="type=rect;num=${num}">${"Rectangle"+num}</option>`;
            num++;
        });
        num = 1;
        circs.forEach(c=>{

            report += makeFViewEntry({
                name: "Circle" + num, 
                summary: `Center: x = ${c.cx + c.units}, y = ${c.cy + c.units}<br/>
                          Radius = ${c.r + c.units}`,
                numAttr: c.numAttr});
            options += `<option value="type=circ;num=${num}">${"Circle"+num}</option>`;
            num++;
        })
        num = 1;
        paths.forEach(p=>{
            report += makeFViewEntry({
                name: "Path" + num, 
                summary: p.d,
                numAttr: p.numAttr});
            options += `<option value="type=path;num=${num}">${"Path"+num}</option>`;
            num++;
        })
        num = 1;
        groups.forEach(g=>{
            report += makeFViewEntry({
                name: "Group" + num, 
                summary: g.children + " child elements",
                numAttr: g.numAttr});
            options += `<option value="type=group;num=${num}">${"Group"+num}</option>`;
            num++;
        })
        fview.innerHTML = report;
        $("#fview-title").html(data.title);
        $("#fview-desc").html(data.desc);
        $('#svg-ele').html(options);
        document.getElementById('new-title').value = data.title;
        document.getElementById('new-desc').value =data.desc;
    }
    $('#scale-rect-form').submit(e=>{
        e.preventDefault();
        let val = $('#scale-rect').val();
        if(isNaN(val) || val <= 0){
            e.preventDefault();
            return alert("Error: invalid scale factor.");
        }
        let arg = {factor: val, loginData: getLoginData()};
        console.log(curFile)
        
        $.ajax({
            type: 'POST',
            contentType: 'application/json',
            data: JSON.stringify(arg),
            url: '/scalerect/' + curFile,
            success: function(dat){
                alert(dat);
            }
        })
    })
    $('#scale-circ-form').submit(e=>{
        e.preventDefault();
        let val = $('#scale-circ').val();
        if(isNaN(val) || val <= 0){
            e.preventDefault();
            return alert("Error: invalid scale factor.");
        }
        let arg = {factor: val, loginData: getLoginData()};
        console.log(curFile)
        $.ajax({
            type: 'POST',
            contentType: 'application/json',
            data: JSON.stringify(arg),
            url: '/scalecirc/' + curFile,
            success: function(dat){
                alert(dat);
            }
        })
    })
    //EDITING/ADDING ATTRIBUTES FROM FILELOG--------------------------------------------------------
    $('#attr-form').submit((e)=>{
        e.preventDefault();
        let attr = {name: $('#new-attr-key').val(), value: $('#new-attr-value').val()};
        const loginData = getLoginData();
        if(!loginData){
            return alert("Error: invalid database connection data. Log out and try again.");
        }
        let arg = {type: props.type, num: props.num, filename: curFile, attr: attr, loginData: loginData};

        console.log("arg", arg);
        $.ajax({
            type: 'POST',
            url:'/updateattribute',
            contentType: 'application/json',
            data: JSON.stringify(arg),

            success: function(data){
                const res = JSON.parse(data);
                if(res.error){
                    alert("Error: " +res.error);
                }else{
                    alert(res.success);
                    location.reload();
                }
                //update whole page(file log)
            },
            fail: function(data){
                alert("Error: updates not saved.");
            }
        });
    });
    //edit svg title attr  form
    $('#td-form').submit((e)=>{

        e.preventDefault();
        let arg = {update: {title: $("#new-title").val(), desc: $('#new-desc').val(), filename: 'uploads/'+curFile}};
        const loginData = getLoginData();
        if(!loginData){
            alert("Error: invalid database connection data. Log out and try again.");
        }
        arg.loginData = loginData;
        console.log(arg);

        $.ajax({
            type: 'POST',
            url:'/updatetd',
            dataType: 'application/json',
            contentType: 'application/json',
            data: JSON.stringify(arg),

            success: function(data){
                const res = JSON.parse(data);
                console.log('data..',data);
                if(res.error){
                    alert("Error: " +res.error);
                }else{
                    alert(res.success);
                    location.reload();
                }
                //update whole page(file log)
            },
            fail: function(data){
                alert("Error: updates not saved.");
            }
        });
    });


    document.getElementById("svg-ele").onchange = (e)=>{
        const data = document.getElementById("svg-ele").value;
        if(document.getElementById("svg-ele").value === "placeholder"){
            $('#attr-summary').html("");
            toggleAttrForm(false);
            return;
        }
        toggleAttrForm(true);
        console.log(data);
        const keypairs = data.split(";");
        const propsJSON =`{"type": "${keypairs[0].split("=")[1]}", "num": "${keypairs[1].split("=")[1]}", "filename": "${curFile}"}`;
        props = JSON.parse(propsJSON);
        //get attribute for selected element
        $.ajax({
            type: 'POST',
            data: propsJSON,
            url: '/attributes',
            contentType: 'application/json',
            success: function(data){
                let str = "";
                const attr = JSON.parse(data).attr;
                attr.forEach(a=>{
                    str+=`<p style = 'font-weight:normal'>${a.name}: ${a.value}</p>`;
                })
                $('#attr-summary').html(str);
            },
            fail: (data)=>{

            }
        })
    }

    //create new svg form
    $('#create-form').submit(function(e){
        let data = {name: $('#new-fname').val()};
        //input check 
        if(data.name.indexOf(".svg") == -1){
            e.preventDefault();
            return alert("Error: invalid file name.");
        }
        
        const loginData = getLoginData();
        if(!loginData){
            alert("Error: invalid database connection data. Log out and try again.");
        }
        data.loginData = loginData;

        $.ajax({
            type: 'POST',
            contentType: 'application/json',
            url: '/create',
            data: JSON.stringify(data),
            success: function(data){
                if(data.error){
                    alert("Error: " + data.error);
                    e.preventDefault();
                }else{
                    alert(data.success);
                }
            },
            fail: function(err){
                alert("Error: " + err.error);
            }
        });
    })
    //get DB status 
    $('#dbstatus').click(function(e){
        $.ajax({
            type: 'POST',
            contentType: 'application/json',
            url: '/status',
            data: JSON.stringify(getLoginData()),
            success: function(data){
                console.log(data);
                if(data.success){
                    alert(data.success);
                }
            }  
        })
    })
    //clear DB data 
    $('#cleardata').click(function(e){
        $.ajax({
            type: 'DELETE',
            contentType: 'application/json',
            url: '/cleardata',
            data: JSON.stringify(getLoginData()),
            success: function(data){
                console.log(data);
                if(data.success){
                    alert(data.success);
                }
            }
            
        })
    })
    $('#qtype').change(function(e){
        const qtype = $('#qtype').val();
        let innerHTML = "";
        switch(qtype){
            case 'creation-date':
                innerHTML = "<hr/><p>Display all files created within a date range.</p><hr/>" + $('#qf-date').html();
                break;
            
            case 'modification-date': 
                innerHTML = "<hr/><p>Display all files modified within a date range.</p><hr/>" + $('#qf-date').html();
                break;
            
            case 'shape-count':
                innerHTML = `<hr/><p>Filter files by their shape count.</p><hr/>`+ $('#qf-shape-count').html();                
                 break;

            case 'most-downloaded':
                innerHTML = `
                <hr/><p>Display the N most frequently downloaded files.</p><hr/>
                <label for = "most-downloaded-num">Display top </label> 
                <input  id = "most-downloaded-num" type = "text" required style = "max-width:75px" placeholder = "(Number)"/>
                <label for = "most-downloaded-num"> most downloaded files</label> 
                `;
                break;

            case 'changes':
                innerHTML = `<hr/><p>Display all changes to a specific file between specific dates.</p><hr/>` + $('#qf-change').html() + '<br/>'+ $('#qf-date').html();
                break;

            case 'allfiles':
                innerHTML = '<hr/><p>Display all files in the database.</p><hr/>'
                break;
        }
        $('#queryoptions').html(innerHTML);

    })
    //execute query
    $('#execute-query').submit(function(e){
        e.preventDefault();
        const url = $('#qtype').val();
        const vqf = validateQf();
        if(!vqf.valid)
            return alert(vqf.error);

        const constraints = getConstraints(url);
        console.log(constraints);

        $.ajax({
            type: 'POST',
            url: '/query/' + url,
            contentType: 'application/json',
            data: JSON.stringify({loginData: getLoginData(), constraints}),
            success: function(data){
                buildQueryTable(data.allRecords);
                $('#table-title').html(url);
            }
        })
    })    
    //edit shape form
    $('#shape-form').submit(function(e){
        e.preventDefault();
        let arg = { rect: null, circ: null, loginData: getLoginData()};
        if(!arg.loginData){
            return alert("Error: invalid db connection data. Try logging in again.");
        }
        console.log(arg.loginData);
        if($('#Rect').is(':checked')){ 
            arg.rect = {x: $('#x').val(), y: $('#y').val(), w: $('#width').val(), h: $('#height').val(), numAttr: 0, units: $('#units-r').val()};
            
            if(isNaN(arg.rect.x) || isNaN(arg.rect.y) || isNaN(arg.rect.w) || isNaN(arg.rect.h)){
                e.preventDefault();
                return alert("Invalid input: unexpected nonnumeric input.");
            }
            console.log(arg.rect);
        }
        if($('#Circ').is(':checked')){
            arg.circ = {cx:$('#cx').val(),cy: $('#cy').val(), r:$('#radius').val(), numAttr: 0, units:$('#units-c').val() }
            if(isNaN(arg.circ.cx) || isNaN(arg.circ.cy) || isNaN(arg.circ.r)){
                e.preventDefault();
                return alert("Invalid input: uenxpected nonnumeric input.");
            }
        }
        $.ajax({
            type: 'POST',
            url: '/addshape/' + curFile,
            contentType: 'application/json',
            data: JSON.stringify(arg),
            success: (d)=>{
               console.log(d);
               if(d){
                alert(d);
               }
            },
            fail: (err)=>{
                alert(err.error);
            }
        })
    })
    //HELPERS --------------------------------------------------------------------------------------
    /*Build table based on query data >> array of all file records, represented as objects */
    function buildQueryTable(data){
        console.log('data', data);
        $('#query-table').empty();

        if(!data || data.length == 0)
            return $('#query-table').append('<h6>No records found.</h6>');
                
        //make table heading
        const fields = Object.keys(data[0]);
        let innerTH = "";
        fields.forEach((field)=> innerTH +=`<th>${field}</th>`);
        $('#query-table').append(innerTH)

        //populate table rows
        data.forEach((recordObj)=>{
            let innerTR = "";
            for(let i = 0; i < fields.length; i++){
                innerTR += `<td>${recordObj[fields[i]]}</td>`;
            }
            $('#query-table').append(`<tr>${innerTR}</tr>`);
        })
    }
    function makeFViewEntry(ele){
        let entry = 
        `<tr>
            <td width = "20%">${ele.name? ele.name:""}</td>
            <td>
                ${ele.summary}
            </td>
            <td>${ele.numAttr}</td>
        </tr>`;
        return entry;
    }
    function toggleAttrForm(enabled){
        $('#new-attr-key').attr('disabled', !enabled);
        $('#new-attr-value').attr('disabled', !enabled);
        $('#new-attr-subm').attr('disabled', !enabled);
    }
    function toggleTDForm(enabled){
        $('#new-title').attr('disabled', !enabled);
        $('#new-desc').attr('disabled', !enabled);
        $('#new-td-subm').attr('disabled', !enabled);
    
        $('#Rect').attr('disabled', !enabled);
        $('#Circ').attr('disabled', !enabled);

        $('#scale-circ').attr('disabled', !enabled);
        $('#scale-rect').attr('disabled', !enabled);
        $('#submscaler').attr('disabled', !enabled);
        $('#submscalec').attr('disabled', !enabled);

    }
    $('#Rect').change(function(){
        if($('#Rect').is(':checked')){
            toggleAddRect(true);
            $('#shape-subm').attr('disabled', false);
        }else{
    
            if(!$('#Circ').is(':checked')){
                $('#shape-subm').attr('disabled', true);
            }
            toggleAddRect(false);
        }
    })
    $('#Circ').change(function(){
        if($('#Circ').is(':checked')){
            toggleAddCirc(true);
            $('#shape-subm').attr('disabled', false);
        }else{
            if(!$('#Rect').is(':checked')){
                $('#shape-subm').attr('disabled', true);
            }
            toggleAddCirc(false);
        }
    })

    function toggleAddRect(enabled){
        $('#x').attr("disabled", !enabled);
        $('#y').attr("disabled", !enabled);
        $('#width').attr("disabled", !enabled);
        $('#height').attr("disabled", !enabled);
        $('#units-r').attr("disabled", !enabled);
    }

    function toggleAddCirc(enabled){
        $('#cx').attr("disabled", !enabled);
        $('#cy').attr("disabled", !enabled);
        $('#radius').attr("disabled", !enabled);
        $('#units-c').attr("disabled", !enabled);
    }

    //return db connection data; null if invalid
    function getLoginData(){
        const loginData = {
            host: $('#dblogin-hostname').val(),
            user: $('#dblogin-uname').val(),
            password: $('#dblogin-pw').val(),
            database: $('#dblogin-dbname').val()
        }
        if(loginData.host === "" || loginData.user === "" || loginData.password === "" || loginData.database === ""){
            return null;
        }
        return loginData;
    }
    //returns constraints for qf input 
    function getConstraints(qtype){
        switch(qtype){
            case 'allfiles':
                return {};
            
            case 'creation-date': 
                return{datesInterval: [$('#date-low').val(), $('#date-high').val()]};
            
            case 'modification-date': 
                return{datesInterval: [$('#date-low').val(), $('#date-high').val()]};

            case 'shape-count':
                let toReturn = {rectRange: null, circRange: null, pathRange: null, groupRange: null};
                if($('#n_rect-low').val() !== ""){
                    toReturn.rectRange = [$('#n_rect-low').val(), $('#n_rect-high').val()];
                }
                if($('#n_circ-low').val() !== ""){
                    toReturn.circRange = [$('#n_circ-low').val(), $('#n_circ-high').val()];
                }
                if($('#n_path-low').val() !== ""){
                    toReturn.pathRange = [$('#n_path-low').val(), $('#n_path-high').val()];
                 }
                if($('#n_group-low').val() !== ""){
                    toReturn.groupRange = [$('#n_group-low').val(), $('#n_group-high').val()];
                }
                return toReturn;
            
            case 'most-downloaded':
                return {display_num: $('#most-downloaded-num').val()};
            
            case 'changes':
                return {fileName: $('#qf-filename').val(), changeType: $('#qf-changetype').val(), changeSort: $('#qf-changesort').val(), datesInterval: [$('#date-low').val(), $('#date-high').val()]}
        }
    }
    //returns true if qf input is valid
    function validateQf(){
        const qtype = $('#qtype').val();
        const toReturn = {valid: false, error: null};
        switch(qtype){
            case 'allfiles':
                return {valid: true, error: null};

            case 'creation-date':
                return validateQfDates();

            case 'modification-date':
                return validateQfDates();
            
            case 'shape-count': 
                return validateQfCounts();

            case 'most-downloaded':
                const inpt = $('#most-downloaded-num').val();
                if(inpt == parseInt(inpt) && inpt >0)
                    return {valid: true, error: null};

                return {valid: false, error: 'ERROR: input must be postive integer.'};
            
            case 'changes':
                const filename = $('#qf-filename').val();
                if(filename.substr(filename.length - 4) !== '.svg')
                    return {valid: false, error: 'ERROR: Invalid filename. (Must have .svg extension)'};
                return validateQfDates();
            }        
    }
    function validateQfDates(){
        if($('#date-high').val() < $('#date-low').val())
            return {valid: false, error: 'ERROR: Start date is later than end date.'};
        
        return {valid: true, error: null};
    }
    function validateQfCounts(){
        if(!checkNumInputs($('#n_rect-low').val(), $('#n_rect-high').val()))
            return {valid: false, error: 'Invalid rectangle input. (Positive integers, min <= max)'};
        if(!checkNumInputs($('#n_circ-low').val(), $('#n_circ-high').val()))
            return {valid: false, error: 'Invalid circle input. (Positive integers, min <= max)'};
        if(!checkNumInputs($('#n_path-low').val(), $('#n_path-high').val()))
            return {valid: false, error: 'Invalid path input. (Positive integers, min <= max)'};
        if(!checkNumInputs($('#n_group-low').val(), $('#n_group-high').val()))
            return {valid: false, error: 'Invalid group input. (Positive integers, min <= max)'};


        return {valid: true, error: null};
    }
    //helper fx: return bool for if inputs are valid for num range
    function checkNumInputs(n_low, n_high){
        console.log(n_low, n_high);

        if(n_low === "" && n_high === "")
            return true;
    
        if(n_low == parseInt(n_low) && n_high ==parseInt(n_high) && n_low <= n_high && n_low >= 0){
            return true;
        }
        return false;
    }
});
