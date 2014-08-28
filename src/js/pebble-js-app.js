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

function sleep(millis, callback) {
    setTimeout(function()
            { callback(); }
    , millis);
}

function fetchWeather(latitude, longitude) {
  var response;
  var req = new XMLHttpRequest();
  req.open('GET', "http://api.openweathermap.org/data/2.5/weather?" + "lat=" + latitude + "&lon=" + longitude, true);
	
  console.log("http://api.openweathermap.org/data/2.5/weather?" + "lat=" + latitude + "&lon=" + longitude);
  req.onload = function(e) {
    if (req.readyState == 4) {
      if(req.status == 200) {
        console.log(req.responseText);
        response = JSON.parse(req.responseText);
		  console.log("Response: " + response);
        var temperature, icon, city, address, sunset;
        //if (response > 0) {
          	var weatherResultList = response.weather[0];
			temperature = response.main.temp;
          	icon = iconFromWeatherId(weatherResultList.id);
          	city = response.name;
		  	sunset = response.sys.sunset;
		  	address = "http://api.openweathermap.org/data/2.5/weather?" + "lat=" + latitude + "&lon=" + longitude;
          	console.log("It is " + temperature + " degrees");
		  	console.log("You are currently residing in: " +city);
		 	console.log("Icon resource loaded: " + icon);
		  	//Incase of future location error, save the working address, longitude and latitude
		  	localStorage.setItem("address1", address);
			localStorage.setItem("latitude1", latitude);
			localStorage.setItem("longitude1", longitude);
		  	console.log("Stored working location at: " + localStorage.getItem("address1"));
			console.log("Working latitude: " + localStorage.getItem("latitude1"));
			console.log("Working longitude: " + localStorage.getItem("longitude1"));
			
          Pebble.sendAppMessage({
            "icon":icon,
            "temperature":temperature,
			});
        //}
      } else {
		  console.log("Error: could not connect! (is api.openweathermap.com down?)");
      }
    }
  };
  req.send(null);
}

function locationSuccess(pos) {
  var coordinates = pos.coords;
	console.log("Location success! Fetching weather...");
	if(fetched == 0){
  		fetchWeather(coordinates.latitude, coordinates.longitude);
		fetched = 1;
	}
	else if(fetched == 1){
		return;
	}
}

function locationError(err) {
  console.warn('Location error (' + err.code + '): ' + err.message);
	var temperatureError = parseInt(err.code) + 400;
	var workingLatitude = localStorage.getItem("latitude1");
	var workingLongitude = localStorage.getItem("longitude1");
	
	fetchWeather(workingLatitude, workingLongitude);
	console.log("Fetching previous working temperature from latitude: " + workingLatitude + " and longitude: " + workingLongitude);
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

// URL to your configuration page
var config_url = "http://edwinfinch.github.io/configscreen-futuraplus";

Pebble.addEventListener("showConfiguration", function(e) {
	var url = config_url;
	console.log("Opening configuration page: " + url);
	Pebble.openURL(url);
});

Pebble.addEventListener("webviewclosed", function(e) {
	if(e.response) {
		var values = JSON.parse(decodeURIComponent(e.response));

		for(key in values) {
			window.localStorage.setItem(key, values[key]);
		}

		Pebble.sendAppMessage(values,
			function(e) {
				console.log("Successfully sent options to Pebble");
			},
			function(e) {
				console.log("Failed to send options to Pebble.\nError: " + e.error.message);
			}
		);
	}
});
