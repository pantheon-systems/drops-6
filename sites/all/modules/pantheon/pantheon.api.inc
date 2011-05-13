<?php
/**
 * @file
 * Pantheon API functions.
 */

// API server and credentials
define('PANTHEON_API_HOST', 'https://api.getpantheon.com');
define('PANTHEON_API_PORT', 8443);
define('PANTHEON_API_SITE_CERT', '/etc/pantheon/system.pem');
define('PANTHEON_API_ROOT_CERT', '/etc/ssl/certs/pantheon.pem');

/**
 * Basic API request function.
 */
function pantheon_api_request($path, $method = 'GET', $data = NULL) {
  $url = PANTHEON_API_HOST .'/'. $path;

  // create a new cURL resource
  $ch = curl_init();
  
  // set URL and other appropriate options
  $opts = array(
    CURLOPT_URL => $url,
    CURLOPT_HEADER => 1,
    CURLOPT_PORT => PANTHEON_API_PORT,
    CURLOPT_RETURNTRANSFER => 1,
    CURLOPT_CAINFO => PANTHEON_API_ROOT_CERT,
    CURLOPT_SSLCERT => variable_get('pantheon_system_cert', PANTHEON_API_SITE_CERT),
    CURLOPT_HTTPHEADER => array('Content-Type: application/json'),
  );
  curl_setopt_array($ch, $opts);
  
  // Handle alternative methods.
  // TODO: Delete?
  if ($method == 'POST') {
    curl_setopt($ch, CURLOPT_POST, 1);
  }
  elseif ($method == 'PUT') {
    curl_setopt($ch, CURLOPT_CUSTOMREQUEST, 'PUT');
  }
  
  if ($data) {
    curl_setopt($ch, CURLOPT_POSTFIELDS, $data);
  }
  
  // Process url
  $result = curl_exec($ch);
  
  if (curl_errno($ch) != 0) {
    $error = curl_error($ch);
    curl_close($ch);
    watchdog('pantheon', 'Curl error: !error accessing !url', array('!error' => $error, '!url' => $url), WATCHDOG_ALERT);
    return FALSE;
  }

  list($headers, $body) = explode("\r\n\r\n", $result);
  
  $return = array(
    'headers' => $headers,
    'body' => $body,
  );

  // close cURL resource, and free up system resources
  curl_close($ch);
  
  return $return;
}

/**
 * Function to get an authorization token
 *
 * This will only allow a site to get tokens for itself, not other sites.
 */
function pantheon_api_get_authorization($token) {
  $token = check_plain($token);
  $poison = cache_get('pantheon-poison-'. $token);
  if ($poison != 0) {
    return FALSE;
  }
  $result = pantheon_api_request('sites/self/authorizations/'. $token);
  if (strpos($result['headers'], 'HTTP/1.1 200') !== FALSE) {
    $authorization = json_decode($result['body']);
    if ($authorization->expires < time() || $authorization->headers->{'X-AuthProxy-Request-Service'} != 'drupal_login') {
      return FALSE;
    }
    else {
      return $authorization;
    }
  }
  else {
    return FALSE;
  }
}