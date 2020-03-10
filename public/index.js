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
            console.log(allSvgs); 
            let files = allSvgs.files;
            for(let i = 0; i < files.length; i++){
                //build table row 
                let row = `<tr>
                    <th>
                        <img id = "flog${i}"/>
                    </th>
                    <th ><a id="fname${i}" download/></th>
                    <th id = "fsize${i}"></th>
                    <th></th>
                    <th></th>
                    <th></th>
                    <th></th>
                    </tr>`

                fileLog.insertAdjacentHTML('beforeend', row);
            }
            let options = "";
            for(let i = 0; i < files.length; i++){
                document.getElementById(`flog${i}`).src = files[i].fileName;
                document.getElementById(`fname${i}`).innerHTML = files[i].fileName;
                document.getElementById(`fname${i}`).href = files[i].fileName;
                document.getElementById(`fsize${i}`).innerHTML = files[i].fileSize;
                
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
        $("#file-view-img").src = svgName;
    });
});
