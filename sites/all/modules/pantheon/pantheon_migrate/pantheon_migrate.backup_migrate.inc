<?php

/**
 * An extension of the backup_migrate_destination class to save a Pantheon approved version of the site
 * Adapted from Ronan's work at https://github.com/ronan/backup_migrate_archive
 */
class backup_migrate_destination_pantheon_migrate extends backup_migrate_destination {
  var $supported_ops = array('source');

  function type_name() {
    return t('Pantheon Archive Source');
  }

  /**
   * Declare the current files directory as a backup source..
   */
  function destinations() {
    $destination = backup_migrate_create_destination('pantheonarchive', array('destination_id' => 'pantheonarchive', 'location' => '.', 'name' => t('Pantheon Archive')));
    return array('pantheonarchive' => $destination);
  }

  /**
   * Return a list of backup filetypes.
   */
  function file_types() {
    return array(
      'tar' => array(
        'extension' => 'tar',
        'filemime' => 'application/x-tar',
        'backup' => TRUE,
        'restore' => FALSE,
      ),
    );
  }

  /**
   * Backup from this source.
   */
  function backup_to_file($file, $settings) {
    if ($this->check_libs()) {

      // Exclude Files That We Should Not Back Up
      $exclude = array();
      $exclude[] = realpath('.') . '/' . str_replace(realpath('.') . '/', '', realpath(file_directory_path()) . '/backup_migrate/pantheon');
      $sites = opendir('sites');
      while ($site = readdir($sites)) {
        switch('sites/' . $site) {
          case 'sites/.':
          case 'sites/all':
          case str_replace(realpath('.') . '/', '', realpath(conf_path())):
          case 'sites/..':
            // Keep These Sites 
            break;
          default:
            $exclude[] = realpath('.') . '/sites/' . $site;
            break;
        }
      }

      $files = $this->get_files_to_backup($this->get_location(), $settings, $exclude, realpath('.') . '/');
      if ($files) {
        $db = $this->get_db();
    
        // TODO - compress this with gzip 
        $file->push_type('tar');
        $gz = new Archive_Tar($file->filepath(), FALSE);

        $gz->addModify($files, $file->name .'/', $this->get_location());
        $gz->addModify($db, $file->name . '/', dirname($db));

        unlink($db);
        rmdir(dirname($db));

        return $file;
      }
      return FALSE;
    }
    return FALSE;
  }

  /**
   * Get a database dump to add to the archive.
   */
  function get_db() {
    backup_migrate_include('destinations', 'files', 'filters', 'profiles');

    $file = new backup_file();
    $settings = _backup_migrate_profile_saved_default_profile();
    $settings->source_id = 'db';
    $settings->filters['compression'] = 'none';
    $file = backup_migrate_filters_backup($file, $settings);

    // Generate a tmp file with the correct final title (because ArchiveTar doesn't seem to allow renaming).
    $tmpdir = realpath(file_directory_temp()) .'/'. uniqid('backup_migrate_');
    mkdir($tmpdir);
    $filepath = $tmpdir . '/database.sql';
    rename($file->filepath(), $filepath);

    return $filepath;
  }

  /**
   * Convert an associated array to an ini format string.
   */
  function _array_to_ini($data, $prefix = '') {
    $content = ''; 
    foreach ($data as $key => $val) {
      if ($prefix) {
        $key = $prefix . '[' . $key .']';
      }
      if (is_array($val)) {
        $content .= $this->_array_to_ini($val, $key);
      }
      else {
        $content .= $key . ' = \''. $val .'\'\n';
      }
    }
    return $content;
  }


   /**
   * Get a list of files to backup from the given set if dirs. Exclude any that match the array $exclude.
   */
  function get_files_to_backup($dir, $settings, $exclude = array(), $base_dir = '') {
    $out = array();
    if (!file_exists($dir)) {
      backup_migrate_backup_fail('Directory %dir does not exist.', array('%dir' => $dir), $settings);
      return FALSE;
    }
    if ($handle = @opendir($dir)) {
      while (($file = readdir($handle)) !== FALSE) {
        if ($file != '.' && $file != '..' && !in_array($dir . '/' . $file, $exclude)) {
          $original_file = $file;
          if ($file = realpath($dir. '/'. $file)) {
            // The file is good
          }
          else {
            $file = ($dir == '.') ? realpath('.') . '/' . $original_file : '/' . realpath('.') . $dir . '/' . $original_file;
          }
          if (is_dir($file)) {
            $subdir = $this->get_files_to_backup($file, $settings, $exclude, $base_dir);
            // If there was an error reading the subdirectory then abort the backup.
            if ($subdir === FALSE) {
              closedir($handle);
              return FALSE;
            }
            $out = array_merge($out, $subdir);
          } else {
            if (is_readable($file)) {
              $out[] = str_replace($base_dir, '', $file);
            }
            else {
              if ($dir == '.') {
                backup_migrate_backup_fail('The directory cannot be backed up because the file %file cannot be read by the web server.', array('%file' => $file), $settings);
              }
              else {
                backup_migrate_backup_fail('The directory cannot be backed up because the file %file cannot be read by the web server.', array('%file' => str_replace($base_dir, '', $file)), $settings);
              }
              closedir($handle);
              return FALSE;
            }
          }
        }
      }
      closedir($handle);
    }
    else {
      backup_migrate_backup_fail('Could not open directory %dir', array('%dir' => $dir), $settings);
      return FALSE;
    }
    return $out;
  }

  /**
   * Break the excpluded paths string into a usable list of paths.
   */
  function get_excluded_paths($paths) {
    $out = explode('\n', $paths);
    foreach ($out as $key => $val) {
      $out[$key] = trim($val, '/ \t\r\n');
    }
    return $out;
  }

  /**
   * Check that the required libraries are installed.
   */
  function check_libs() {
    $result = true;

    // Extend inlcude path with path to this module's includes directory.
    $includes_directory = './' . drupal_get_path('module', 'pantheon_migrate');
    $include_path_old = set_include_path($includes_directory . ';' . get_include_path());

    // Check if PEAR.php is present in a non-fatal way and error gracefully if it isn't.
    include('PEAR.php');

    if (!class_exists('PEAR')) {
      _backup_migrate_message('PEAR is not installed correctly. See the README.txt file in the pantheon_migrate module directory for help.', array(), 'error');
      $result = false;
    }

    // Check if Tar.php is present in a non-fatal way and error gracefully if it isn't.
    if ($result) {
      // Try to get version in this module's includes directory first, but prevent warning texts being output.
      if (file_exists($includes_directory . '/Tar.php')) {
        include_once $includes_directory . '/Tar.php';
      }
      if (!class_exists('Archive_Tar')) {
        // Try to get via PEAR directory structure.
        include_once 'Archive/Tar.php';
        if (!class_exists('Archive_Tar')) {
          _backup_migrate_message('Archive_Tar is not installed correctly. See the README.txt file in the pantheon_migrate module directory for help.', array(), 'error');
          $result = false;
        }
      }
    }

    // Restore include path.
    set_include_path($include_path_old);
    return $result;
  }
}

/**
 * A desintation type for sending files out to Pantheon
 */
class backup_migrate_destination_files_pantheon_storage extends backup_migrate_destination_files_pantheon {
  var $supported_ops = array('manual backup', 'restore', 'list files', 'configure', 'delete');
  function __construct($params = array()) {
    $dir = file_directory_path() .'/backup_migrate/pantheon';
    parent::__construct($params + array('location' => $dir, 'name' => t('Pantheon Directory')));
  }
}

/**
 * A destination type for sending files out to Pantheon
 */
class backup_migrate_destination_files_pantheon extends backup_migrate_destination {
  var $supported_ops = array('scheduled backup', 'manual backup', 'restore', 'list files', 'configure', 'delete');

  function type_name() {
    return t("Pantheon Directory");
  }

  /**
   * Override the action links for a file on a given destination.
   */
  function get_file_links($file_id) { 
    $out = array();

    // Don't display the download/delete/restore ops if they are not available for this destination.
    $can_read       = $this->can_read_file($file_id);
    $can_delete     = $this->can_delete_file($file_id);

    $destination_id = $this->get_id();
    if ($can_read && user_access("access backup files")) {
      $out[] = l(t('download'), 'admin/content/backup_migrate/destination/downloadfile/'. $destination_id .'/'. $file_id);
    }
    if ($can_read && user_access("access backup files")) {
      $out[] = l(
        t('send to pantheon'), 
        'https://getpantheon.com/get-pantheon', 
        array(
          'attributes' => array('target' => '_blank'), 
          'query' => 'import=' . urlencode(url('admin/content/backup_migrate/destination/downloadfile/' . $destination_id .'/'. $file_id . '/' . pantheon_migrate_token_get($file_id), array('query' => array('noise' => rand(rand(100, 1000), rand(5000, 50000))), 'absolute' => TRUE))),
        )
      );
    }
    if ($can_delete && user_access("delete backup files")) {
      $out[] = l(t('delete'), 'admin/content/backup_migrate/destination/list/deletefile/'. $destination_id .'/'. $file_id, array('query' => 'destination=admin/content/backup_migrate/export/pantheon'));
    }
    return $out;
  }

  /**
   * File save destination callback.
   */
  function save_file($file, $settings) {
    if ($dir = $this->get_location()) {
      if ($this->check_dir($dir)) {
        $filepath = rtrim($dir, "/") ."/". $file->filename();
        rename($file->filepath(), $filepath);

        // chmod, chown and chgrp the file if needed.
        if ($chmod = $this->settings('chmod')) {
          if (!@chmod($filepath, octdec($chmod))) {
            _backup_migrate_message('Unable to set the file mode for: @file', array('@file' => $filepath), 'error');
          }
        }
        if ($chgrp = $this->settings('chgrp')) {
          if (!@chgrp($filepath, $chgrp)) {
            _backup_migrate_message('Unable to set the file group for: @file', array('@file' => $filepath), 'error');
          }
        }
        return $file;
      }
    }
  }

  /**
   * Determine if we can read the given file.
   */
  function can_read_file($file_id) {
    return $this->op('restore') && is_readable($this->get_filepath($file_id));
  }

  /**
   * File load destination callback.
   */
  function load_file($file_id) {
    $filepath = $this->get_filepath($file_id);
    if (file_exists($filepath)) {
      backup_migrate_include('files');
      return new backup_file(array('filepath' => $filepath));
    }
  }

  /**
   * Get the file object for the given file.
   */
  function get_file($file_id) {
    $files = $this->list_files();
    if (isset($files[$file_id])) {
      isset($files[$file_id]);
    }
    return NULL;
  }

  /**
   * File list destination callback.
   */
  function list_files() {
    $files = array();
    if ($dir = $this->get_location()) {
      if ($handle = @opendir($dir)) {
        backup_migrate_include('files');
        while (FALSE !== ($file = readdir($handle))) {
          $filepath = $dir ."/". $file;
          $files[$file] = new backup_file(array('filepath' => $filepath));
        }
      }
    }
    return $files;
  }

  /**
   * File delete destination callback.
   */
  function delete_file($file_id) {
    $filepath = $this->get_filepath($file_id);
    file_delete($filepath);
  }

  /**
   * Get the filepath from the given file id.
   */
  function get_filepath($file_id) {
    $filepath = rtrim($this->get_location(), '/') .'/'. $file_id;
    return $filepath;
  }

  /**
   * Get the form for the settings for the files destination.
   */
  function edit_form() {
    $form = parent::edit_form();
    $form['location'] = array(
      "#type" => "textfield",
      "#title" => t("Directory path"),
      "#default_value" => $this->get_location(),
      "#required" => TRUE,
      "#description" => t('Enter the path to the directory to save the backups to. Use a relative path to pick a path relative to your Drupal root directory. The web server must be able to write to this path.'),
    );
    $form['settings'] = array(
      '#type' => 'fieldset',
      '#title' => t('Advanced Settings'),
      '#tree' => TRUE,
      '#collapsible' => TRUE,
      '#collapsed' => TRUE,
    );
    if (function_exists('chmod')) {
      $form['settings']['chmod'] = array(
        '#type' => 'textfield',
        '#title' => t('Change file mode (chmod)'),
        '#size' => 5,
        '#default_value' => $this->settings('chmod'),
        '#description' => t('If you enter a value here, backup files will be chmoded with the mode you specify. Specify the mode in octal form (e.g. 644 or 0644) or leave blank to disable this feature.'),
      );
    }
    if (function_exists('chgrp')) {
      $form['settings']['chgrp'] = array(
        '#type' => 'textfield',
        '#title' => t('Change file group (chgrp)'),
        '#size' => 5,
        '#default_value' => $this->settings('chgrp'),
        '#description' => t('If you enter a value here, backup files will be chgrped to the group you specify. Leave blank to disable this feature.'),
      );
    }
    return $form;
  }

  /**
   * Validate the form for the settings for the files destination.
   */
  function edit_form_validate($form, &$form_state) {
    $values = $form_state['values'];
    if (isset($values['settings']['chmod']) && !empty($values['settings']['chmod']) && !preg_match('/0?[0-7]{3}/', $values['settings']['chmod'])) {
      form_set_error('chmod', t('You must enter a valid chmod octal value (e.g. 644 or 0644) in the change mode field, or leave it blank.'));
    }
    parent::edit_form_validate($form, $form_state);
  }

  /**
   * Submit the form for the settings for the files destination.
   */
  function edit_form_submit($form, &$form_state) {
    // Add a 0 to the start of a 3 digit file mode to make it proper PHP encoded octal.
    if (strlen($form_state['values']['settings']['chmod']) == 3) {
      $form_state['values']['settings']['chmod'] = '0' . $form_state['values']['settings']['chmod'];
    }
    parent::edit_form_submit($form, $form_state);
  }

  /**
   * Prepare the destination directory for the backups.
   */
  function check_dir($directory) {
    $out = TRUE;
    $dirs = array();

    // If the directory doesn't exist try to create it.
    if (!is_dir($directory)) {
      foreach (explode('/', $directory) as $dir) {
        $dirs[] = $dir;
        $path = implode($dirs, '/');
        if ($path && !is_dir(realpath($path)) && !file_check_directory($path, FILE_CREATE_DIRECTORY)) {
          $out = FALSE;
        }
      }

      if (!$out || !file_check_directory($directory)) {
        // Unable to create destination directory.
        _backup_migrate_message("Unable to create or write to the save directory '%directory'. Please check the file permissions that directory and try again.", array('%directory' => $directory), "error");
        return FALSE;
      }
    }

    // If the destination directory is within the webroot, then secure it as best we can.
    if ($this->dir_in_webroot($directory)) {
      $directory = $this->check_web_dir($directory);
    }

    return $directory;
  }
  
  /**
   * Check that a web accessible directory has been properly secured, othewise attempt to secure it.
   */
  function check_web_dir($directory) {
    // Setup htaccess for password authentication
    $htaccess_lines = "order allow,deny\ndeny from all\n";
    if (!is_file($directory .'/.htaccess') || !is_readable($directory .'/.htaccess') || strpos(file_get_contents($directory .'/.htaccess'), $htaccess_lines) === FALSE) {

      // Attempt to protect the backup files from public access using htaccess.
      if (($fp = @fopen($directory .'/.htaccess', 'w')) && @fputs($fp, $htaccess_lines)) {
        fclose($fp);
        chmod($directory .'/.htaccess', 0664);
      }
      // Unable to create htaccess... warn the user.
      else {
        $message = "Security warning: Couldn't modify .htaccess file. Please create a .htaccess file in your %directory directory which contains the following lines: <code>!htaccess</code> or add them to the existing .htaccess file";
        $replace = array('%directory' => $directory, '!htaccess' => '<br />'. nl2br(check_plain($htaccess_lines)));
        drupal_set_message(t($message, $replace), "error");
        watchdog('security', t($message, $replace), WATCHDOG_ERROR);
        return FALSE;
      }
    }

    // Check the user agent to make sure we're not responding to a request from drupal itself.
    // That should prevent infinite loops which could be caused by poormanscron in some circumstances.
    if (strpos($_SERVER['HTTP_USER_AGENT'], 'Drupal') !== FALSE) {
      return FALSE;
    }

    // Check to see if the destination is publicly accessible
    $test_contents = "this file should not be publicly accesible";
    // Create the the text.txt file if it's not already there.
    if (!is_file($directory .'/test.txt') || file_get_contents($directory .'/test.txt') != $test_contents) {
      if ($fp = fopen($directory .'/test.txt', 'w')) {
        @fputs($fp, $test_contents);
        fclose($fp);
      }
      else {
        $message = t("Security notice: Backup and Migrate was unable to write a test text file to the destination directory %directory, and is therefore unable to check the security of the backup destination. Backups to the server will be disabled until the destination becomes writable and secure.", array('%directory' => $directory));
        drupal_set_message($message, "error");
        return FALSE;
      }
    }

    // Attempt to read the test file via http. This may fail for other reasons,
    // so it's not a bullet-proof check.
    $path = trim(drupal_substr($directory .'/test.txt', drupal_strlen(file_directory_path())), '\\/');
    if ($this->test_file_readable_remotely($path, $test_contents)) {
      $message = t("Security notice: Backup and Migrate will not save backup files to the server because the destination directory is publicly accessible. If you want to save files to the server, please secure the '%directory' directory", array('%directory' => $directory));
      drupal_set_message($message, "error");
      return FALSE;
    }

    return $directory;
  }

  /**
   * Check if the given directory is within the webroot and is therefore web accessible.
   */
  function dir_in_webroot($directory) {
    if (strpos(realpath($directory), realpath($_SERVER['DOCUMENT_ROOT'])) !== FALSE) {
      return TRUE;
    }
    return FALSE;
  }

  /**
   * Check if a file can be read remotely via http.
   */
  function test_file_readable_remotely($path, $contents) {
    $url = $GLOBALS['base_url'] .'/'. file_directory_path() .'/'. str_replace('\\', '/', $path);
    $result = drupal_http_request($url);
    if (!empty($result->data) && strpos($result->data, $contents) !== FALSE) {
      return TRUE;
    }
    return FALSE;
  }

  /**
   * Check if a file returns a 404 error
   */
  function test_file_exist_remotely($path, $test_contents) {
    $url = $GLOBALS['base_url'] .'/'. file_directory_path() .'/'. str_replace('\\', '/', $path);
    $result = drupal_http_request($url);
    if ($result->code == '404') {
      return TRUE;
    } 
    return FALSE;
  }
}
