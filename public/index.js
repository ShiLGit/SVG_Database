// Put all onload AJAX calls here, and event listeners
$(document).ready(function() {
    const fileLog = document.getElementById('file-log');

    // On page-load AJAX Example
    $.ajax({
        type: 'get',            //Request type
        dataType: 'json',       //Data type - we will use JSON for almost everything 
        url: '/someendpoint',   //The server endpoint we are connecting to
        data: {
            name1: "Value 1",
            name2: "Value 2"
        },
        success: function (data) {
            /*  Do something with returned object
                Note that what we get is an object, not a string, 
                so we do not need to parse it on the server.
                JavaScript really does handle JSONs seamlessly
            */
          //  $('#blah').html("On page load, received string '"+data.foo+"' from server");
            //We write the object to the console to show that the request was successful
            console.log(data); 

        },
        fail: function(error) {
            // Non-200 return, do something with error
            $('#blah').html("On page load, received error from server");
            console.log(error); 
        }
    });
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
                    <th>
                        <img src = "${files[i].fileName}"/>
                    </th>
                    <th ><a href = "${files[i].fileName}" download/>${files[i].fileName}</th>
                    <th>${files[i].fileSize}</th>
                    <th>${files[i].numRect}</th>
                    <th>${files[i].numCirc}</th>
                    <th>${files[i].numPaths}</th>
                    <th>${files[i].numGroups}</th>
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

    //fileviewer: choose img to display
    $('#file-select-form').submit(function(e){
        e.preventDefault();
        let svgName =document.getElementById("svg-name").value 
        $("#file-view-img").attr("src",svgName);

        $.ajax({
                type: 'get',            //Request type
                dataType: 'json',       //Data type - we will use JSON for almost everything 
                url: '/uploads/' + svgName,   //The server endpoint we are connecting to
                success: function(data){
                    console.log("...",data);
                }
        })
    });
});
