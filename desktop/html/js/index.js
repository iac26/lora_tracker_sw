

//create the map
var map = new L.Map('map', {
    crs: L.CRS.EPSG3857,
    continuousWorld: true,
    worldCopyJump: false
  });
  var url = 'https://wmts20.geo.admin.ch/1.0.0/ch.swisstopo.pixelkarte-farbe/default/current/3857/{z}/{x}/{y}.jpeg';
  var tilelayer = new L.tileLayer(url);
  map.addLayer(tilelayer);
  map.setView(L.latLng(46.57591, 7.84956), 8);

let user_pos = L.circle([50.5, 30.5], {radius: 200}).addTo(map);
let user_head = L.polyline([[50.5, 30.5], [50.5, 30.5]], {color: 'blue'}).addTo(map);
const location_options = {
    enableHighAccuracy: false,
    timeout: 2000,
    maximumAge: Infinity,
};
  
function first_location_success(pos) {
    const crd = pos.coords;

    console.log("Your current position is:");
    console.log(`Latitude : ${crd.latitude}`);
    console.log(`Longitude: ${crd.longitude}`);
    console.log(`More or less ${crd.accuracy} meters.`);
    map.setView(L.latLng(crd.latitude, crd.longitude), 9);
    user_pos.setLatLng(L.latLng(crd.latitude, crd.longitude));
    user_pos.setRadius(crd.accuracy)
}

function first_location_error(err) {
    console.warn(`ERROR(${err.code}): ${err.message}`);
}

//oneShot get location
navigator.geolocation.getCurrentPosition(first_location_success, first_location_error, location_options);
  

let id;


function user_loc_success(pos) {
  const crd = pos.coords;
  //console.log("Your current position is:");
  //console.log(`Latitude : ${crd.latitude}`);
  //console.log(`Longitude: ${crd.longitude}`);
  //console.log(`More or less ${crd.accuracy} meters.`);
  user_pos.setLatLng(L.latLng(crd.latitude, crd.longitude));
  if(crd.heading) {
    let vertices = [[crd.latitude, crd.longitude], [crd.latitude+crd.accuracy*Math.sin(crd.heading)*1.2, crd.longitude+crd.accuracy*Math.cos(crd.heading)*1.2]];
    user_head.setLatLngs(vertices);
  }
  user_pos.setRadius(crd.accuracy)

}

function user_loc_error(err) {
  console.error(`ERROR(${err.code}): ${err.message}`);
}

id = navigator.geolocation.watchPosition(user_loc_success, user_loc_error, location_options);



let last_track = 0;

let local_data = [];
let known_board_id = [];
let tracks = {};

get_data();


function make_popup(data) {
    return `<h4>Tracker ${data["board_id"]}<h4/><p>Batterie: ${data["bat_level"]}</p><p>Coordonnées: ${data["latitude"]}, ${data["longitude"]} (${data["hdop"]})</p><p>Vitesse: ${data["velocity"]}</p><p>Dernier Packet: ${data["time"]}</p>`;
}


function update_map(new_data) {
    //create missing tracks
    new_data.forEach(element => {
        if(!known_board_id.includes(element["board_id"])) {
            //create new track
            known_board_id.push(element["board_id"]);
            tracks[element["board_id"]] = {
                "board_id": element["board_id"],
                "line"    : L.polyline([[element["latitude"], element["longitude"]]], {color: 'red'}).addTo(map),
                "point"   : L.circle([element["latitude"], element["longitude"]], {color: 'red'}).addTo(map),
            };
            tracks[element["board_id"]]["line"].bindPopup(make_popup(element)).addTo(map);
        } else {
            if((element["latitude"] > 44.0 && element["latitude"] < 47.0) && (element["longitude"] > 5.5 && element["longitude"] < 8.5)) {
                //known id -> add track and update front
                let prev_line = tracks[element["board_id"]]["line"].getLatLngs();
                prev_line.push([element["latitude"], element["longitude"]]);
                tracks[element["board_id"]]["line"].setLatLngs(prev_line);
                tracks[element["board_id"]]["line"].setPopupContent(make_popup(element));
                tracks[element["board_id"]]["point"].setLatLng([element["latitude"], element["longitude"]]);
            } //weird coordinates get discarded...
        } 
    });

}

function get_data() {
    $.post("download_track.php",
        {
            user_token: "iacopolebg",
            last_track: last_track
        },
        function (data, status) {
            console.log(data);

            data.forEach(element => {
                local_data.push(element);
                last_track = element["id"];
            });

            update_map(data);


            setTimeout(get_data, 5000);
        });
}

