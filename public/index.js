// Put all onload AJAX calls here, and event listeners
$(document).ready(function() {
    const fileLog = document.getElementById('file-log');
    //get all files for file laaaaaaaaaaaaaaaaaawg; render on screen
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

//FILEVIEWER STUFFFFFFFF----------------------------------------------------------------------------------------
    $('#file-select-form').submit(function(e){
        e.preventDefault();
        let svgName =document.getElementById("svg-name").value 
        $("#file-view-img").attr("src",svgName);

        $.ajax({
                type: 'get',            //Request type
                dataType: 'json',       //Data type - we will use JSON for almost everything 
                url: '/uploads/' + svgName,   //The server endpoint we are connecting to
                success: function(data){
                    renderReport(data);

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
    }

    //EDITING/ADDING ATTRIBUTES FROM FILELOG--------------------------------------------------------
    document.getElementById("svg-ele").onchange = (e)=>{
        const data = document.getElementById("svg-ele").value;
        console.log(document.getElementById("svg-ele").value);


        if(document.getElementById("svg-ele").value === "placeholder"){
            $('#attr-text').html("");
            return;
        }
        const keypairs = data.split(";");
        const props = {type: keypairs[0].split("=")[1], num: keypairs[1].split("=")[1]};
        

        //get attribute for selected element
        $.ajax({
            type: 'get',
            dataType: 'json',
            url: '/attributes/rects.svg',
            success: (data)=>{

            },
            fail: (data)=>{

            }
        })
    }

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
        console.log('...?');
        $('#new-attr-key').attr('disabled', !enabled);
        $('#new-attr-value').attr('disabled', !enabled);
        $('#new-attr-subm').attr('disabled', !enabled);
    }
});
