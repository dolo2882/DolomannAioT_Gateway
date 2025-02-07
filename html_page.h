#ifndef HTML_PAGES_H
#define HTML_PAGES_H

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>WiFi 設定</title>
  <meta charset="UTF-8">
  <style>
    body {
      font-family: Arial, sans-serif;
    }
    .container {
      margin: 50px auto;
      max-width: 400px;
      padding: 20px;
      border: 1px solid #ccc;
      border-radius: 5px;
    }
    .form-group {
      margin-bottom: 20px;
    }
    label {
      display: block;
      margin-bottom: 5px;
    }
    input[type="text"],
    input[type="password"] {
      width: 100%;
      padding: 8px;
      font-size: 16px;
    }
    button {
      padding: 10px 20px;
      background-color: #007bff;
      color: #fff;
      border: none;
      border-radius: 5px;
      cursor: pointer;
    }
    button:hover {
      background-color: #0056b3;
    }
  </style>
</head>
<body>
  <div class="container">
    <h1>WiFi 設定</h1>
    <form id="wifiForm">
      <div class="form-group">
        <label for="ssid">SSID:</label>
        <input type="text" id="ssid" name="ssid" required>
      </div>
      <div class="form-group">
        <label for="password">密碼:</label>
        <input type="password" id="password" name="password" required>
      </div>
      <div class="form-group">
        <button type="submit">儲存</button>
      </div>
    </form>
  </div>

  <script>
    document.getElementById('wifiForm').addEventListener('submit', function(event) {
      event.preventDefault(); // Prevent form submission
      
      // Get form values
      var ssid = document.getElementById('ssid').value;
      var password = document.getElementById('password').value;

      // Send form data to server
      var xhr = new XMLHttpRequest();
      xhr.open('POST', '/save', true);
      xhr.setRequestHeader('Content-Type', 'application/x-www-form-urlencoded');
      xhr.onreadystatechange = function() {
        if (xhr.readyState === XMLHttpRequest.DONE && xhr.status === 200) {
          alert('WiFi 設定已儲存！');
          location.reload(); // Reload page after saving settings
        }
      };
      xhr.send('ssid=' + encodeURIComponent(ssid) + '&password=' + encodeURIComponent(password));
    });
  </script>
</body>
</html>
)rawliteral";

#endif
