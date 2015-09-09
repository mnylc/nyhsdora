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
        'map_directory' => array(
          'description' => 'The main MARC directory which contains the maps and broadside data.',
          'required' => TRUE,
        ),
        'broadside_directory' => array(
          'description' => 'The main MARC directory which contains the maps and broadside data.',
          'required' => TRUE,
        ),
        'alexander_directory' => array(
          'description' => 'The William Alexander Pages directory.',
          'required' => TRUE,
        ),
        'namespace' => array(
          'description' => 'The namespace to use for the ingested objects. Defaults to islandora.',
        ),
      ),
      'examples' => array(
        'drush -u 1 nyhsdora_migrate_all --map_directory=/pathtomapmods/ --broadside_directory=/pathtobroadsidemods/ --alexander_directory=/pathtoalexanderpapers/' => 'Migrate all content.',
        'drush -u 1 nyhs_migrate_all --map_directory=/pathtomapmods/ --broadside_directory=/pathtobroadsidemods/ --alexander_directory=/pathtoalexanderpapers/ --namespace=my_namespace' => 'Migrate all content.',
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
    'type' => 'directory',
    'namespace' => drush_get_option('namespace', 'islandora'),
    'parent_relationship_pred' => 'isMemberOfCollection',
    'parent_relationship_uri' => FEDORA_RELS_EXT_URI,
  );

  // Migrate Maps Collection.
  $parameters['parent'] = NYHSDORA_MAPS_PID;
  $parameters['target'] = drush_get_option('map_directory');
  $map_preprocessor = new NyhsdoraBatchPreprocessor($connection, $parameters);
  islandora_batch_handle_preprocessor($map_preprocessor);

  // Migrate Broadside Collection.
  $parameters['parent'] = NYHSDORA_BROADSIDE_PID;
  $parameters['target'] = drush_get_option('broadside_directory');
  $broadside_preprocessor = new NyhsdoraBatchPreprocessor($connection, $parameters);
  islandora_batch_handle_preprocessor($broadside_preprocessor);

  // Migrate William Alexander Pages.
  $parameters['parent'] = NYHSDORA_AMERICAN_EXPERIENCE_PID;
  $parameters['target'] = drush_get_option('alexander_directory');
  $alexander_papers_preprocessor = new NyhsdoraAlexanderBatchPreprocessor($connection, $parameters);
  islandora_batch_handle_preprocessor($alexander_papers_preprocessor);
}
