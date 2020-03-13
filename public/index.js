import { finalizingTransformersModule } from "javascript-obfuscator/src/container/modules/node-transformers/FinalizingTransformersModule";

// Put all onload AJAX calls here, and event listeners
$(document).ready(function() {
    const fileLog = document.getElementById('file-log');
    let curFile = ""; //file open in file viewer
    let props = {}; //properties of selected omponent

    updateLog();
    toggleAddCirc(false);
    toggleAddRect(false);
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
                            <img src = "${files[i].fileName}"/>
                        </td>
                        <td ><a href = "${files[i].fileName}" download/>${files[i].fileName}</td>
                        <td>${files[i].fileSize}</td>
                        <td>${files[i].numRect}</td>
                        <td>${files[i].numCirc}</td>
                        <td>${files[i].numPaths}</td>
                        <td>${files[i].numGroups}</td>
                        </tr>`
    
                    fileLog.insertAdjacentHTML('beforeend', row);
                }
                let options = "";
                for(let i = 0; i < files.length; i++){           
                    options += `<option value="${files[i].fileName}">${files[i].fileName}</option>`;
                }
                document.getElementById("svg-name").innerHTML = options;
                 
            },
            fail: function(error) {
                console.log(error); 
            }
        })
    }
//FILEVIEWER STUFFFFFFFF----------------------------------------------------------------------------------------
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

    //EDITING/ADDING ATTRIBUTES FROM FILELOG--------------------------------------------------------
    $('#attr-form').submit((e)=>{
        e.preventDefault();
        let attr = {name: $('#new-attr-key').val(), value: $('#new-attr-value').val()};
        let props2 = JSON.parse(props);
        props2.attr = attr;
        console.log('props2:', props2);
        $.ajax({
            type: 'POST',
            url:'/updateattribute',
            contentType: 'application/json',
            data: JSON.stringify(props2),

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

    $('#td-form').submit((e)=>{
        e.preventDefault();
        let arg = {title: $("#new-title").val(), desc: $('#new-desc').val(), filename: 'uploads/'+curFile};
        console.log(arg);

        $.ajax({
            type: 'POST',
            url:'/updatetd',
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
        const keypairs = data.split(";");
        props = `{"type": "${keypairs[0].split("=")[1]}", "num": "${keypairs[1].split("=")[1]}", "filename": "${curFile}"}`;
        
        console.log(props);
        //get attribute for selected element
        $.ajax({
            type: 'POST',
            data: props,
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
    $('#create-form').submit(function(e){
        let data = {name: $('#new-fname').val()};
        //input check 
        if(data.name.indexOf(".svg") == -1){
            data.name = data.name.concat(".svg");
        }
        console.log(data);

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

            }
        });
    })
    $('#uploadForm').submit(function() {
        alert(".");
        $(this).ajaxSubmit({
            error: function(xhr) {
                alert(xhr.status);
                status('Error: ' + xhr.status);
            },
            success: function(response) {
                alert(response);
                console.log(response);
            }
        });
        //Very important line, it disable the page refresh.
        return false;
    }); 
    $('#shape-form').submit(function(e){
        e.preventDefault();
        let arg = { rect: null, circ: null};
        if($('#Rect').is(':checked')){ 
            arg.rect = {x: $('#x').val(), y: $('#y').val(), w: $('#width').val(), h: $('#height').val(), numAttr: 0, units: $('#units-r').val()};
            console.log(arg.rect);
        }
        if($('#Circ').is(':checked')){
            arg.circ = {cx:$('#cx').val(),cy: $('#cy').val(), r:$('#radius').val(), numAttr: 0, units:$('#units-c').val() }
        
        }
        $.ajax({
            type: 'POST',
            url: '/addshape/' + curFile,
            contentType: 'application/json',
            dataType: 'application/json',
            data: JSON.stringify(arg),
            success: (d)=>{
                alert();
                console.log(d);
            },
            fail: (err)=>{
                alert("Error: " + err);
            }
        })
    })
    //HELPERS --------------------------------------------------------------------------------------
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
});
