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
    'nyhsdora_update_pdf_quality' => array(
      'description' => 'Replaces the OBJs on all page objects split apart from a PDF and triggers new derivative creation.',
      'options' => array(
        'dryrun' => array(
          'description' => 'Runs the batch as a dryrun not commiting any actual changes.',
          'required' => FALSE,
        ),
      ),
      'examples' => array(
        'drush -u 1 nyhsdora_update_pdf_quality',
      ),
      'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_LOGIN,
    ),
    'nyhsdora_update_mods' => array(
      'description' => dt("Updates NYHS' MODS metadata."),
      'drupal dependencies' => array(
        'islandora',
      ),
      'options' => array(
        'dryrun' => array(
          'description' => dt('Whether we are doing a test run or not, defaults to FALSE if not specified.'),
          'required' => FALSE,
        ),
      ),
    ),
    'nyhsdora_restore_mods' => array(
      'description' => dt("Restores NYHS' MODS metadata."),
      'drupal dependencies' => array(
        'islandora',
      ),
      'examples' => array(
        'drush -u 1 -v nyhsdora_restore_mods',
      ),
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

/**
 * Goes out and re-creates all the page object's datastreams from a PDF.
 */
function drush_nyhsdora_update_pdf_quality() {
  $dryrun = drush_get_option('dryrun', FALSE);
  $tuque = islandora_get_tuque_connection();
  $query = <<<EOQ
SELECT DISTINCT ?parent FROM <#ri> WHERE {
  ?obj <fedora-model:hasModel> <info:fedora/islandora:pageCModel> ;
       <fedora-model:label> ?label ;
       <fedora-rels-ext:isMemberOf> ?parent .
  ?parent <fedora-view:disseminates> ?parent_ds ;
          <http://contentdm.org/ontology/relsext#pointer> ?pointer .
  ?parent_ds <fedora-view:disseminationType> <info:fedora/*/PDF>
  OPTIONAL {
    ?obj <fedora-view:disseminates> ?ds .
    ?ds <fedora-view:disseminationType> <info:fedora/*/CDM_ITEM>
  }
FILTER(!bound(?ds))
}
EOQ;
  $results = $tuque->repository->ri->sparqlQuery($query);
  foreach ($results as $result) {
    $book_pid = $result['parent']['value'];
    batch_set(nyhsdora_update_pdf_quality_update_create_batch($book_pid, $dryrun));
  }
  drush_backend_batch_process();
}

/**
 * Helper function to create a batch to process a series of pages.
 *
 * @param string $book_pid
 *   The PID of the book object whose children are being updated.
 * @param bool $dryrun
 *   FALSE if no changes are to be committed, TRUE otherwise.
 *
 * @return array
 *   An array describing the batch operation to be ran.
 */
function nyhsdora_update_pdf_quality_update_create_batch($book_pid, $dryrun) {
  return array(
    'operations' => array(
      array('nyhsdora_update_pdf_quality_update_creation_batch_operation', array(
        $book_pid,
        $dryrun,
        ),
      ),
    ),
    'title' => t('Re-creating OBJs for pages...'),
    'init_message' => t('Preparing to update pages.'),
    'progress_message' => t('Time elapsed: @elapsed <br/>Estimated time remaining @estimate.'),
    'error_message' => t('An error has occurred.'),
    'file' => drupal_get_path('module', 'nyhsdora') . '/nyhsdora.drush.inc',
  );
}

/**
 * Batch operation for re-creating assets for pages of a book that was split.
 *
 * @param string $book_pid
 *   The PID of the book object whose children are being updated.
 * @param bool $dryrun
 *   FALSE if no changes are to be committed, TRUE otherwise.
 * @param array $context
 *   The context of the current batch operation.
 */
function nyhsdora_update_pdf_quality_update_creation_batch_operation($book_pid, $dryrun, &$context) {
  module_load_include('inc', 'islandora_paged_content', 'includes/utilities');
  module_load_include('inc', 'islandora', 'includes/utilities');

  $book_object = islandora_object_load($book_pid);
  $sandbox = &$context['sandbox'];
  if (!isset($sandbox['pages'])) {
    $sandbox['files'] = array();
    $pages = islandora_paged_content_get_pages($book_object);
    $sandbox['pages'] = $pages;
    // Store a copy of the PDF to be used by all pages.
    $file_uri = file_create_filename('nyhspdf', 'temporary://');
    $book_object['PDF']->getContent($file_uri);
    $temp_file = islandora_temp_file_entry($file_uri);
    $sandbox['book_pdf'] = $temp_file;
    // Get the colorspace of the PDF now.
    $file_path = drupal_realpath($temp_file->uri);
    exec(escapeshellcmd("identify -format %[colorspace], $file_path"), $output);
    $values = reset($output);
    $colorspaces = array_filter(explode(',', $values));

    foreach ($sandbox['pages'] as &$page) {
      // Sequence numbers start at 1, colorspaces keyed at 0.
      $selected_colorspace = $colorspaces[$page['page'] - 1];
      if (strcasecmp($selected_colorspace, 'rgb') == 0 || strcasecmp($selected_colorspace, 'srgb') == 0) {
        $device = 'tiff48nc';
      }
      elseif (strcasecmp($selected_colorspace, 'cmyk') == 0) {
        $device = 'tiff64nc';
      }
      else {
        drush_log(dt('Encountered a grey scale PDF for @pid?', array('@pid' => $book_object->id)));
        $device = 'tiffgray';
      }
      $page['device'] = $device;
    }
    unset($page);
  }
  $page = array_shift($sandbox['pages']);
  $context['message'] = t('Processing page @page of @book.', array(
    '@page' => $page['pid'],
    '@book' => $book_object->id,
  ));
  // Delete all existing datastreams which are being derived again. We are
  // deleting the RELS-INT such that the width/height get re-generated.
  $undeleteable = array(
    'RELS-EXT',
    'DC',
  );
  $page_object = islandora_object_load($page['pid']);
  if (!$dryrun) {
    foreach ($page_object as $dsid => $ds) {
      if (!in_array($dsid, $undeleteable)) {
        $page_object->purgeDatastream($dsid);
      }
    }
  }
  // Create a new OBJ given the sequence of pages here.
  $pdf_file = $sandbox['book_pdf'];
  if (!$dryrun) {
    $pdf = islandora_paged_content_chop_pdf($pdf_file->uri, $page['page']);
    if ($pdf) {
      islandora_paged_content_update_datastream($page_object, $pdf, 'PDF', 'PDF', 'application/pdf', 'M');
      $page_pdf_file = islandora_temp_file_entry($pdf);
      $sandbox['files'][] = $page_pdf_file;
    }
    else {
      watchdog('nyhsdora', 'Unable to chop a PDF from the PDF for @pid.', array('@pid' => $page['pid']));
    }

    $tif = islandora_paged_content_extract_tiff_from_pdf($pdf_file->uri, $page['page'], $page['device'], '600');
    if ($tif) {
      islandora_paged_content_update_datastream($page_object, $tif, 'OBJ', 'OBJ Datastream', 'image/tiff', 'M', FALSE);
      $tif_file = islandora_temp_file_entry($tif);
      $sandbox['files'][] = $tif_file;
    }
    else {
      watchdog('nyhsdora', 'Unable to extract a TIF from the PDF for @pid.', array('@pid' => $page['pid']));
    }
    $full_text = islandora_paged_content_extract_text_from_pdf($pdf_file->uri, $page['page']);
    if ($full_text) {
      islandora_paged_content_update_datastream($page_object, $full_text, 'OCR', 'OCR', 'text/plain', 'M', FALSE);
      $full_text_file = islandora_temp_file_entry($full_text);
      $sandbox['files'][] = $full_text_file;
    }
    else {
      watchdog('nyhsdora', 'Unable to extract text from the PDF for @pid.', array('@pid' => $page['pid']));
    }

    // Trigger gearman.
    islandora_job_derivatives_islandora_object_ingested($page_object);
  }
  if (empty($sandbox['pages'])) {
    // Clean up the stored PDF and files now.
    file_delete($sandbox['book_pdf']);
    foreach ($sandbox['files'] as $file) {
      file_delete($file);
    }
    $context['finished'] = 1;
    drush_log(dt('Processing complete for book @pid.', array('@pid' => $book_object->id)));
  }
  else {
    $context['finished'] = 0;
  }
}

/**
 * Command callback to update NYHS' MODS metadata.
 */
function drush_nyhsdora_update_mods() {
  batch_set(nyhsdora_update_mods_metadata_batch(drush_get_option('dryrun', FALSE)));
  drush_backend_batch_process();
}

/**
 * Constructs a batch to update MODS metadata.
 */
function nyhsdora_update_mods_metadata_batch($dryrun) {
  return array(
    'operations' => array(
      array(
        'nyhsdora_update_mods_batch_operation',
        array($dryrun),
      ),
    ),
    'title' => t('Updating MODS metadata...'),
    'init_message' => t('Preparing to update MODS metadata.'),
    'progress_message' => t('Time elapsed: @elapsed <br/>Estimated time remaining @estimate.'),
    'error_message' => t('An error has occurred.'),
  );
}

/**
 * Batch operation to update MODS metadata.
 */
function nyhsdora_update_mods_batch_operation($dryrun, &$context) {
  $sandbox = &$context['sandbox'];
  drupal_static_reset('islandora_get_tuque_connection');
  $tuque = islandora_get_tuque_connection();
  $query = <<<EOQ
SELECT ?object ?created FROM <#ri> WHERE {
  ?disseminationType <fedora-view:disseminationType>  <info:fedora/*/MODS> .
  ?object <info:fedora/fedora-system:def/view#disseminates> ?disseminationType ;
          <fedora-model:createdDate> ?created
  !filter
}
ORDER BY ?created

EOQ;
  if (!isset($sandbox['total'])) {
    $sandbox['progress'] = 0;
    $sandbox['total'] = $tuque->repository->ri->countQuery(format_string($query, array(
      '!filter' => '',
    )), 'sparql');
    if ($sandbox['total'] == 0) {
      $context['finished'] = 1;
      $context['message'] = t('Nothing to update.');
      return;
    }
    $sandbox['result_stash'] = array();
  }
  if (empty($sandbox['result_stash'])) {
    $limited_query = format_string($query, array(
      '!filter' => (isset($sandbox['last_date']) ?
          "FILTER(?created > \"{$sandbox['last_date']}\"^^<xsd:dateTime>)" :
          ''),
    )) . <<<EOQ
LIMIT 1000
EOQ;
    drush_log(dt('Querying for 1000 objects.'));
    $sandbox['result_stash'] = $tuque->repository->ri->sparqlQuery($limited_query);
    if (empty($sandbox['result_stash'])) {
      // Ran out of items early?
      $context['finished'] = 1;
      $context['message'] = t('Updated @count PID(s) and ran out of items early... Somebody manually updated a document?', array(
        '@count' => $sandbox['total'],
      ));
      return;
    }
  }
  $result = array_shift($sandbox['result_stash']);
  $updating_pid = $result['object']['value'];
  $sandbox['last_date'] = $result['created']['value'];
  $transform_path = drupal_get_path('module', 'nyhsdora') . '/xml/nyhs_update_mods.xsl';
  $xsl = new DOMDocument();
  $xsl->load($transform_path);
  $xslt = new XSLTProcessor();
  $xslt->importStylesheet($xsl);
  $object = islandora_object_load($updating_pid);
  if ($object) {
    // Run XSL and set content.
    $dom = new DOMDocument();
    $dom->loadXML($object['MODS']->content);
    $result = $xslt->transformToXml($dom);
    if ($result) {
      if (!$dryrun) {
        $object['MODS']->content = $result;
      }
      drush_log(dt('Updated MODS for @pid.', array('@pid' => $updating_pid)));
    }
    else {
      drush_log(dt('Failed to transform MODS for @pid.', array('@pid' => $updating_pid)));
    }
  }
  else {
    drush_log(dt('Unable to load object for @pid.', array('@pid' => $updating_pid)));
  }
  $sandbox['progress'] = min($sandbox['total'], $sandbox['progress'] + 1);
  $context['finished'] = $sandbox['progress'] / $sandbox['total'];
  $context['message'] = t('Updated @progress of @total.', array(
    '@progress' => $sandbox['progress'],
    '@total' => $sandbox['total'],
  ));
}


/**
 * Command callback to restore NYHS' MODS metadata.
 */
function drush_nyhsdora_restore_mods() {
  batch_set(array(
    'operations' => array(array('nyhsdora_restore_mods_batch_operation', array())),
    'title' => t('Restoring MODS metadata...'),
    'init_message' => t('Preparing to restore MODS metadata.'),
    'progress_message' => t('Time elapsed: @elapsed <br/>Estimated time remaining @estimate.'),
    'error_message' => t('An error has occurred.'),
  ));
  drush_backend_batch_process();
}


/**
 * Batch operation to restore MODS metadata.
 */
function nyhsdora_restore_mods_batch_operation(&$context) {
  $sandbox = &$context['sandbox'];
  drupal_static_reset('islandora_get_tuque_connection');
  $tuque = islandora_get_tuque_connection();
  $query = <<<EOQ
SELECT ?object ?created FROM <#ri> WHERE {
  ?disseminationType <fedora-view:disseminationType>  <info:fedora/*/MODS> .
  ?object <info:fedora/fedora-system:def/view#disseminates> ?disseminationType ;
          <fedora-model:createdDate> ?created;
          <fedora-rels-ext:isMemberOfCollection>+ <info:fedora/islandora:american_experience>
  !filter
}
ORDER BY ?created

EOQ;
  if (!isset($sandbox['total'])) {
    $sandbox['progress'] = 0;
    $sandbox['total'] = $tuque->repository->ri->countQuery(format_string($query, array(
      '!filter' => '',
    )), 'sparql');
    if ($sandbox['total'] == 0) {
      $context['finished'] = 1;
      $context['message'] = t('Nothing to restore.');
      return;
    }
    $sandbox['result_stash'] = array();
  }
  if (empty($sandbox['result_stash'])) {
    $limited_query = format_string($query, array(
      '!filter' => (isset($sandbox['last_date']) ?
          "FILTER(?created > \"{$sandbox['last_date']}\"^^<xsd:dateTime>)" :
          ''),
    )) . <<<EOQ
LIMIT 10
EOQ;
    drush_log(dt('Querying for 10 objects.'));
    $sandbox['result_stash'] = $tuque->repository->ri->sparqlQuery($limited_query);
    if (empty($sandbox['result_stash'])) {
      // Ran out of items early?
      $context['finished'] = 1;
      $context['message'] = t('Restored @count PID(s) and ran out of items early... Somebody manually updated a document?', array(
        '@count' => $sandbox['total'],
      ));
      return;
    }
  }
  $result = array_shift($sandbox['result_stash']);
  $updating_pid = $result['object']['value'];
  $sandbox['last_date'] = $result['created']['value'];
  $object = islandora_object_load($updating_pid);
  if ($object) {
    if (isset($object['MODS'][1])) {
      $object['MODS']->content = $object['MODS'][1]->content;
    }
    drush_log(dt('Restored MODS for @pid.', array('@pid' => $updating_pid)));
  }
  else {
    drush_log(dt('Unable to load object for @pid.', array('@pid' => $updating_pid)));
  }
  $sandbox['progress'] = min($sandbox['total'], $sandbox['progress'] + 1);
  $context['finished'] = $sandbox['progress'] / $sandbox['total'];
  $context['message'] = t('Restored @progress of @total.', array(
    '@progress' => $sandbox['progress'],
    '@total' => $sandbox['total'],
  ));
}
