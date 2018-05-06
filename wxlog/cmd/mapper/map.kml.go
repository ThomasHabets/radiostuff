package main

import "html/template"

var tmplMap = template.Must(template.New("").Parse(`
<!DOCTYPE html>
<html>
  <head>
    <meta name="viewport" content="initial-scale=1.0">
    <meta charset="utf-8">
    <title>KML Layers</title>
    <style>
      /* Always set the map height explicitly to define the size of the div
       * element that contains the map. */
      #map {
        height: 100%;
      }
      /* Optional: Makes the sample page fill the window. */
      html, body {
        height: 100%;
        margin: 0;
        padding: 0;
      }
    </style>
  </head>
  <body>
    <div id="map"></div>
    <script>

      function initMap() {
        var map = new google.maps.Map(document.getElementById('map'), {
          zoom: 3,
          center: {lat: 20, lng: 0}
        });
      var heatmapData = [
{{range .Data -}}
        {location: new google.maps.LatLng({{.Lat}}, {{.Long}}), weight: {{.Weight}}},
{{end}}
      ];
{{range .Stations -}}
      new google.maps.Marker({
          position: {lat: {{.Lat}}, lng: {{.Long}}},
          map: map,
          title: '{{.Name}}:\n{{range .Seen}}{{.}}\n{{end}}'
      });
{{end}}
      var heatmap = new google.maps.visualization.HeatmapLayer({
      data: heatmapData
      });
      
      heatmap.setMap(map);
      /*
        var ctaLayer = new google.maps.KmlLayer({
          url: 'http://googlemaps.github.io/js-v2-samples/ggeoxml/cta.kml',
          map: map
      });
      */
      }
    </script>
    <script async defer
    src="https://maps.googleapis.com/maps/api/js?key={{.APIKey}}&callback=initMap&libraries=visualization">
    </script>
  </body>
</html>
`))
