<?php
include_once './' . drupal_get_path('module', 'apachesolr') . '/Drupal_Apache_Solr_Service.php';

class Pantheon_Apache_Solr_Service extends Drupal_Apache_Solr_Service {

  protected function _makeHttpRequest($url, $method = 'GET', $headers = array(), $content = '', $timeout = false) {
    // Set a response timeout
    if ($timeout) {
      $default_socket_timeout = ini_set('default_socket_timeout', $timeout);
    }
    // Pantheon completely reconstructs the URL.
    $parts = parse_url($url);
    $host = variable_get('pantheon_index_host', 'index.'. variable_get('pantheon_tier', 'live') .'.getpantheon.com');
    $path = 'sites/self/environments/'. variable_get('pantheon_environment', 'dev') .'/index';
    $url = 'https://'. $host .'/'. $path . str_replace('/solr', '', $parts['path']).'?'.$parts['query'];

    $client_cert = '../certs/binding.pem';
    $port = variable_get('pantheon_index_port', 449);
    $ch = curl_init();
    // Janktastic, but the SolrClient assumes http

    // set URL and other appropriate options
    $opts = array(
      CURLOPT_URL => $url,
      CURLOPT_HEADER => 1,
      CURLOPT_PORT => $port,
      CURLOPT_RETURNTRANSFER => 1,
      CURLOPT_SSLCERT => $client_cert,
      CURLOPT_HTTPHEADER => array('Content-type:text/xml; charset=utf-8', 'Expect:'),
    );
    curl_setopt_array($ch, $opts);

    // If we are doing a delete request...
    if ($method == 'DELETE') {
      curl_setopt($ch, CURLOPT_CUSTOMREQUEST, "DELETE");
    }
    // If we are doing a put request...
    if ($method == 'PUT') {
      curl_setopt($ch, CURLOPT_CUSTOMREQUEST, "PUT");
    }
    // If we are doing a put request...
    if ($method == 'POST') {
      curl_setopt($ch, CURLOPT_POST, 1);
    }
    if ($content != '') {
      curl_setopt($ch, CURLOPT_POSTFIELDS, $content);
    }

    $response = curl_exec($ch);

    if ($response == null) {
      // TODO; better error handling
      watchdog('pantheon_apachesolr', "Error !error connecting to !url on port !port", array('!error' => curl_error($ch), '!url' => $url, '!port' => $port), WATCHDOG_ERROR);
    }
    else {
      // mimick the $result object from drupal_http_request()
      $result = new stdClass();
      list($split, $result->data) = explode("\r\n\r\n", $response, 2);
      $split = preg_split("/\r\n|\n|\r/", $split);
      list($result->protocol, $result->code, $result->status_message) = explode(' ', trim(array_shift($split)), 3);
      // Parse headers.
      $result->headers = array();
      while ($line = trim(array_shift($split))) {
        list($header, $value) = explode(':', $line, 2);
        if (isset($result->headers[$header]) && $header == 'Set-Cookie') {
          // RFC 2109: the Set-Cookie response header comprises the token Set-
          // Cookie:, followed by a comma-separated list of one or more cookies.
          $result->headers[$header] .= ',' . trim($value);
        }
        else {
          $result->headers[$header] = trim($value);
        }
      }
    }

    // Restore the response timeout
    if ($timeout) {
      ini_set('default_socket_timeout', $default_socket_timeout);
    }

    // This will no longer be needed after http://drupal.org/node/345591 is committed
    $responses = array(
      0 => 'Request failed',
      100 => 'Continue', 101 => 'Switching Protocols',
      200 => 'OK', 201 => 'Created', 202 => 'Accepted', 203 => 'Non-Authoritative Information', 204 => 'No Content', 205 => 'Reset Content', 206 => 'Partial Content',
      300 => 'Multiple Choices', 301 => 'Moved Permanently', 302 => 'Found', 303 => 'See Other', 304 => 'Not Modified', 305 => 'Use Proxy', 307 => 'Temporary Redirect',
      400 => 'Bad Request', 401 => 'Unauthorized', 402 => 'Payment Required', 403 => 'Forbidden', 404 => 'Not Found', 405 => 'Method Not Allowed', 406 => 'Not Acceptable', 407 => 'Proxy Authentication Required', 408 => 'Request Time-out', 409 => 'Conflict', 410 => 'Gone', 411 => 'Length Required', 412 => 'Precondition Failed', 413 => 'Request Entity Too Large', 414 => 'Request-URI Too Large', 415 => 'Unsupported Media Type', 416 => 'Requested range not satisfiable', 417 => 'Expectation Failed',
      500 => 'Internal Server Error', 501 => 'Not Implemented', 502 => 'Bad Gateway', 503 => 'Service Unavailable', 504 => 'Gateway Time-out', 505 => 'HTTP Version not supported'
    );

    if (!isset($result->code) || $result->code < 0) {
      $result->code = 0;
    }

    if (isset($result->error)) {
      $responses[0] .= ': ' . check_plain($result->error);
    }

    if (!isset($result->data)) {
      $result->data = '';
    }

    if (!isset($responses[$result->code])) {
      $result->code = floor($result->code / 100) * 100;
    }

    $protocol = "HTTP/1.1";
    $headers[] = "{$protocol} {$result->code} {$responses[$result->code]}";
    if (isset($result->headers)) {
      foreach ($result->headers as $name => $value) {
        $headers[] = "$name: $value";
      }
    }
    return array($result->data, $headers);
  }
  /**
  * Like PHP's built in http_build_query(), but uses rawurlencode() and no [] for repeated params.
  */
  public function httpBuildQuery(array $query, $parent = '') {
    $params = array();

    foreach ($query as $key => $value) {
      $key = ($parent ? $parent : rawurlencode($key));

      // Recurse into children.
      if (is_array($value)) {
        $params[] = $this->httpBuildQuery($value, $key);
      }
      // If a query parameter value is NULL, only append its key.
      elseif (!isset($value)) {
        $params[] = $key;
      }
      else {
        $params[] = $key . '=' . $value;
        if ($key == 'hl.fl') {
          $params[] = 'hl=true';
          $params[] = 'hl.simple.pre=' . rawurlencode('<strong>');
          $params[] = 'hl.simple.post=' . rawurlencode('</strong>');
          $params[] = 'hl.snippets=3';
          $params[] = 'f.content.hl.alternateField=teaser';
          $params[] = 'f.content.hl.maxAlternateFieldLength=256';
          //$params[] = 'f.content.hl.fragmenter=regex';
        }
      }
    }

    return implode('&', $params);
  }
}
