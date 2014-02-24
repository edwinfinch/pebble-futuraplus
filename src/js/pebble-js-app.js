var fetched = 0;

function iconFromWeatherId(weatherId) {
  if (weatherId > 199 && weatherId < 233) {
    return 0; //Thunderstorm
  } 
	else if (weatherId > 299 && weatherId < 322) {
    return 1; //Drizzle
  } 
	else if (weatherId > 499 && weatherId < 523) {
    return 2; //Rain
  } 
	else if (weatherId > 599 && weatherId < 622) {
    return 3; //Snow
  } 
	else if (weatherId > 700 && weatherId < 742) {
	return 4; //Haze or fog
  }
	else if (weatherId == 800){
	return 5; //Clear
  }
	else if (weatherId == 801){
	return 6; //Few clouds
  }
	else if (weatherId > 801){
	return 7; //Scattered, broken, or overcast clouds
  }
}

function fetchWeather(latitude, longitude) {
  var response;
  var req = new XMLHttpRequest();
  req.open('GET', "http://api.openweathermap.org/data/2.1/find/city?" +
    "lat=" + latitude + "&lon=" + longitude + "&cnt=1", true);
	console.log("http://api.openweathermap.org/data/2.1/find/city?" + "lat=" + latitude + "&lon=" + longitude + "&cnt=1")
  req.onload = function(e) {
    if (req.readyState == 4) {
      if(req.status == 200) {
        console.log(req.responseText);
        response = JSON.parse(req.responseText);
        var temperature, icon, city;
        if (response && response.list && response.list.length > 0) {
          var weatherResult = response.list[0];
          temperature = Math.round(weatherResult.main.temp - 273.15);
          icon = iconFromWeatherId(weatherResult.weather[0].id);
          city = weatherResult.name;
          console.log("It is " + temperature + " degrees");
		  console.log("You are currently residing in: " +city);
			console.log("Icon resource loaded: " + icon);
          Pebble.sendAppMessage({
            "icon":icon,
            "temperature":temperature,
            "city":city});
        }
      } else {
        console.log("Error");
      }
    }
  };
  req.send(null);
}

function locationSuccess(pos) {
  var coordinates = pos.coords;
	if(fetched == 0){
  		fetchWeather(coordinates.latitude, coordinates.longitude);
		fetched = 1;
	}
	else if(fetched == 1){
		return;
	}
}

function locationError(err) {
  console.warn('location error (' + err.code + '): ' + err.message);
  Pebble.sendAppMessage({
    "city":"Loc Unavailable",
    "temperature":"N/A"
  });
}

var locationOptions = { "timeout": 15000, "maximumAge": 60000 }; 


Pebble.addEventListener("ready",
                        function(e) {
                          console.log("connect!" + e.ready);
                          locationWatcher = window.navigator.geolocation.watchPosition(locationSuccess, locationError, locationOptions);
                          console.log(e.type);
                        });

Pebble.addEventListener("appmessage",
                        function(e) {
                          window.navigator.geolocation.getCurrentPosition(locationSuccess, locationError, locationOptions);
						  fetched = 0;
                          console.log(e.type);
                          console.log(e.payload.temperature);
                          console.log("message!");
                        });

Pebble.addEventListener("webviewclosed",
                                     function(e) {
                                     console.log("webview closed");
                                     console.log(e.type);
                                     console.log(e.response);
                                     });

/*
Old method of fetching weather
Outdated because it only fetches one pre-defined city
New method fetches location

function HTTPGET(url) {
	console.log("HTTPGET called with url " + url);
	var req = new XMLHttpRequest();
	req.open("GET", url, false);
	req.send(null);
	console.log("Returning response from Openweathermap");
	return req.responseText;
}

var getWeather = function() {
	//Get weather info
	console.log("Getting wheather info");
	var response = HTTPGET("http://api.openweathermap.org/data/2.5/weather?q=Guelph,on");

	//Convert to JSON
	var json = JSON.parse(response);

	//Extract the data
	var temperature = Math.round(json.main.temp - 273.15);
	var location = json.name;
	console.log("Formatting");
	//Console output to check all is working.
	console.log("It is " + temperature + " degrees in " + location + " today!");

	//Construct a key-value dictionary	
	var dict = {"KEY_LOCATION" : location, "KEY_TEMPERATURE": temperature};
	console.log("Dictionary created, sending to pebble");

	//Send data to watch for display
	Pebble.sendAppMessage(dict);
};

Pebble.addEventListener("ready",
  function(e) {
	//App is ready to receive JS messages
	console.log("Ready.");
	getWeather();
  }
);

Pebble.addEventListener("appmessage",
  function(e) {
	//Watch wants new data!
	console.log("Appmessage called");
	getWeather();
  }
);
*/