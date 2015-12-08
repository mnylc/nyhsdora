<?php

/**
 * @file
 * Implementation of Drush hooks.
 */

/**
 * Implements hook_drush_command().
 */
function nyhsdora_drush_command() {
  return array(
    'nyhsdora_migrate_all' => array(
      'aliases' => array('nyhs_migrate_all'),
      'description' => 'Migrate the William Alexander Papers.',
      'options' => array(
        'map' => array(
          'description' => 'The zip file or directory which contains the maps mods data. If using directory set --type to directory.',
          'required' => TRUE,
        ),
        'broadside' => array(
          'description' => 'The zip file or directory which contains the broadside mods data. If using directory set --type to directory.',
          'required' => TRUE,
        ),
        'alexander' => array(
          'description' => 'The William Alexander Pages directory.',
          'required' => TRUE,
        ),
        'type' => array(
          'description' => 'Optional: defaults to zip but if the map and broadside paths are directories instead of zip use directory.',
        ),
        'namespace' => array(
          'description' => 'The namespace to use for the ingested objects. Defaults to islandora.',
        ),
      ),
      'examples' => array(
        'drush -u 1 nyhs_migrate_all --map=/maps.zip --broadside=/broadside.zip --alexander=/pathtoalexanderpapers/' => 'Migrate all content.',
        'drush -u 1 nyhs_migrate_all --map=/pathtomapmods/ --broadside=/pathtobroadsidemods/ --alexander=/pathtoalexanderpapers/ --type=directory' => 'Migrate all content.',
      ),
      'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_LOGIN,
    ),
  );
}

/**
 * Implements hook_drush_command().
 */
function drush_nyhsdora_migrate_all() {
  drupal_static_reset('islandora_get_tuque_connection');
  $connection = islandora_get_tuque_connection();

  // Default parameters used for all 3 collection migrations.
  $parameters = array(
    'type' => drush_get_option('type', 'zip'),
    'namespace' => drush_get_option('namespace', 'islandora'),
    'parent_relationship_pred' => 'isMemberOfCollection',
    'parent_relationship_uri' => FEDORA_RELS_EXT_URI,
  );

  // Migrate Maps Collection.
  $parameters['parent'] = NYHSDORA_MAPS_PID;
  $parameters['target'] = drush_get_option('map');
  $map_preprocessor = new NyhsdoraBatchPreprocessor($connection, $parameters);
  islandora_batch_handle_preprocessor($map_preprocessor);

  // Migrate Broadside Collection.
  $parameters['parent'] = NYHSDORA_BROADSIDE_PID;
  $parameters['target'] = drush_get_option('broadside');
  $broadside_preprocessor = new NyhsdoraBatchPreprocessor($connection, $parameters);
  islandora_batch_handle_preprocessor($broadside_preprocessor);
}
