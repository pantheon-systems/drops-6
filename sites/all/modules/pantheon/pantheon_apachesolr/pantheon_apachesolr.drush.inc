<?php
/**
 * Implementation of hook_drush_command().
 *
 * In this hook, you specify which commands your
 * drush module makes available, what it does and
 * description.
 *
 * Notice how this structure closely resembles how
 * you define menu hooks.
 *
 * See `drush topic docs-commands` for a list of recognized keys.
 *
 * @return
 *   An associative array describing your command(s).
 */
function pantheon_apachesolr_drush_command() {
  $items = array();

  $items['hyperion-push'] = array(
    'description' => "Push the solr schema to hyperion.",
    'arguments' => array(
      'shema' => 'Optional path to schema.xml. Defaults to the schema included in the pantheon_apachesolr module directory.',
    ),
    'examples' => array(
      'drush hyperion-push' => 'Push the schema, creating a core if necessary.',
      'drush hyperion-push path/to/mycustomschema.xml' => 'Push a custom schema.',
    ),
    'aliases' => array('hypush'),
    'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_CONFIGURATION,
  );

  return $items;
}

function drush_pantheon_apachesolr_hyperion_push($schema = NULL) {
  include_once('pantheon_apachesolr.module');
  if ($schema == NULL) {
    $schema = drupal_get_path('module', 'pantheon_apachesolr') .'/schema.xml';
  }
  drush_print(dt('Pushing !schema to hyperion', array('!schema' => $schema)));
  $response = pantheon_apachesolr_update_schema($schema);
  drush_print($response);
}
