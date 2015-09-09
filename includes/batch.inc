<?php

/**
 * @file
 * Integration with islandora_batch.
 */

/**
 * NYHS customizations on the scan batch preprocessor.
 *
 * @see IslandoraScanBatch
 */
class NyhsdoraBatchPreprocessor extends IslandoraScanBatch {
  /**
   * Sets recursive scan to false.
   *
   * @see IslandoraScanBatch::__construct
   */
  public function __construct(IslandoraTuque $connection, array $parameters) {
    parent::__construct($connection, $parameters);
    $this->recursiveScan = FALSE;
  }

  /**
   * Get the associated object class.
   *
   * @see IslandoraScanBatch::getObjectClass()
   */
  protected static function getObjectClass() {
    return 'NyhsdoraBatchObject';
  }
}

/**
 * NYHS customizations for the scan batch object for maps and broadsides.
 *
 * @see IslandoraScanBatchObject
 */
class NyhsdoraBatchObject extends IslandoraScanBatchObject {
  /**
   * Add collection and content model relationships.
   */
  public function addRelationships() {
    $collection = $this->preprocessorParameters['parent'];
    $predicate = $this->preprocessorParameters['parent_relationship_pred'];
    $uri = $this->preprocessorParameters['parent_relationship_uri'];
    $this->relationships->add($uri, $predicate, $collection);
    $this->addContentModelRelationships();
  }

  /**
   * Add the content model relationship(s).
   */
  protected function addContentModelRelationships() {
    $this->models = array('islandora:bookCModel');
  }

  /**
   * Function batch_process.
   */
  public function batchProcess() {
    $mods = $this->getMods();
    if ($mods) {
      unset($this->label);
      $this->label = $this->getTitle();
    }
    $this->getDc();
    return ISLANDORA_BATCH_STATE__DONE;
  }

  /**
   * Get the title for this object.
   *
   * Gets the label not empty; otherwise, grabs a title from the MODS.
   */
  protected function getTitle() {
    if (!isset($this->label) || empty($this->label)) {
      // Acquire a title.
      $mods = $this->getMods();
      if ($mods) {
        $mods_doc = new DOMDocument();
        $mods_doc->loadXML($mods);
        $mods_xpath = new DOMXPath($mods_doc);
        $mods_xpath->registerNamespace('m', 'http://www.loc.gov/mods/v3');
        // Add the part number to the object title.
        $this->label = $mods_xpath->evaluate('string(//m:mods/m:titleInfo/m:title/text())') . '. ' . $mods_xpath->evaluate('string(//m:mods/m:titleInfo/m:partNumber/text())');
      }
    }

    return $this->label;
  }

  /**
   * Function to get the mods.
   */
  protected function getMods() {
    if (!isset($this['MODS'])) {
      module_load_include('inc', 'rochdora', 'includes/utilities');
      $mods_datastream = $this->constructDatastream('MODS', 'M');
      $mods_datastream->mimetype = 'application/xml';
      $mods_datastream->label = 'MODS Record';
      $mods_file = $this->objectInfo['xml']->uri;
      if (!empty($mods_file)) {
        $mods_datastream->setContentFromFile($mods_file);
      }
      else {
        return FALSE;
      }
      $this->ingestDatastream($mods_datastream);
    }
    return $this['MODS']->content;
  }

  /**
   * Function to get dc.
   */
  protected function getDc() {
    if (!isset($this['DC'])) {
      $dc_datastream = $this->constructDatastream('DC', 'M');
      $dc_datastream->mimetype = 'application/xml';
      $dc_datastream->label = 'DC Record';
      $mods_content = $this->getMods();
      if ($mods_content) {
        $dc_datastream->content = static::runXslTransform(
          array(
            'xsl' => $this->modsToDcTransform,
            'input' => $mods_content,
          )
        );
        $this->ingestDatastream($dc_datastream);
      }
      else {
        return FALSE;
      }
    }
    return $this['DC']->content;
  }

  /**
   * Get the pages to ingest from the mods.
   */
  public function getChildren(IslandoraTuque $connection) {
    $children = array();
    // Load file for parsing out children.
    $mods_file = $this->objectInfo['xml']->uri;
    $mods_doc = new DOMDocument();
    $mods_doc->load($mods_file);
    $mods_xpath = new DOMXPath($mods_doc);
    $mods_xpath->registerNamespace('mods', 'http://www.loc.gov/mods/v3');
    $sequence_number = 1;
    // Get the pages from the related items with the type constituent.
    foreach ($mods_xpath->query('/mods:mods/mods:relatedItem[@type="constituent"]') as $element) {
      $page = array();
      $page['label'] = $mods_xpath->evaluate('string(mods:titleInfo/mods:title/text())', $element);
      $page['obj_url'] = $mods_xpath->evaluate('string(mods:location/mods:url/text())', $element);
      $children[] = new NyhsdoraBatchPageObject($connection, $this->id, $sequence_number, $page, $this->preprocessorParameters);
      $sequence_number = $sequence_number + 1;
    }

    return $children;
  }
}

/**
 * NYHS customizations for the scan batch object for map and broadside pages.
 *
 * @see IslandoraScanBatchObject
 */
class NyhsdoraBatchPageObject extends IslandoraScanBatchObject {
  protected $parentId;
  protected $sequenceNumber;
  protected $pageImageUrl;

  /**
   * Constructor.
   */
  public function __construct(IslandoraTuque $connection, $parent_id, $sequence, $object_info, $pp) {
    parent::__construct($connection, $parent_id, $object_info, $pp);
    $this->parentId = $parent_id;
    $this->sequenceNumber = $sequence;
    $this->label = $object_info['label'];
    $this->pageImageUrl = $object_info['obj_url'];
  }

  /**
   * Add collection and content model relationships.
   */
  public function addRelationships() {
    module_load_include('inc', 'islandora_paged_content', 'includes/utilities');
    islandora_paged_content_add_relationships_to_child($this, array(
      'model' => 'islandora:pageCModel',
      'parent' => $this->parentId,
      'page_number' => $this->sequenceNumber,
      'extract_text' => 'none',
    ));
  }

  /**
   * Function batch_process.
   */
  public function batchProcess() {
    $this->getObj();
    return ISLANDORA_BATCH_STATE__DONE;
  }

  /**
   * Function to get the OBJ.
   */
  protected function getObj() {
    if (!isset($this['OBJ'])) {
      // Grab the remote image and save it locally for ingest.
      $file = system_retrieve_file($this->pageImageUrl, 'temporary://', TRUE);
      $obj_datastream = $this->constructDatastream('OBJ', 'M');
      $obj_datastream->label = $file->filename;
      $obj_datastream->mimetype = $file->filemime;
      if ($file !== FALSE) {
        $obj_datastream->setContentFromFile($file->uri);
      }
      else {
        return FALSE;
      }
      $this->ingestDatastream($obj_datastream);
      file_delete($file);
    }
  }
}

/**
 * NYHS customizations on the scan batch preprocessor.
 *
 * @see IslandoraScanBatch
 */
class NyhsdoraAlexanderBatchPreprocessor extends IslandoraScanBatch {
  /**
   * Sets recursive scan to false.
   *
   * @see IslandoraScanBatch::__construct
   */
  public function __construct(IslandoraTuque $connection, array $parameters) {
    parent::__construct($connection, $parameters);
    $this->recursiveScan = FALSE;
  }

  /**
   * Get the associated object class.
   *
   * @see IslandoraScanBatch::getObjectClass()
   */
  protected static function getObjectClass() {
    return 'NyhsdoraAlexanderBatchObject';
  }


  /**
   * Group file entries logically, to pass off to the import object.
   *
   * Default implementation groups on files' basename.
   *
   * @param array $files
   *   An array, as returned by file_scan_directory().
   */
  protected function groupFiles($files) {
    $grouped = array();
    $pages = array();
    // TODO: Group according to basename, to and hand-off each group to object
    // wrappers.
    foreach ($files as $uri => &$file_object) {
      $file_object->ext = pathinfo($file_object->filename, PATHINFO_EXTENSION);
      // Scan the tif objects as pages, but keep the xml for creating the book.
      if ($file_object->ext != 'xml') {
        // Clean up filename so the pages can be sorted and ingested in order.
        $name_index = str_replace(array('_oldNG051116m', 'm', 'n'), '', $file_object->name);
        $pages[$name_index][$file_object->ext] = $file_object;
      }
      else {
        $grouped[$file_object->name][$file_object->ext] = $file_object;
      }
    }
    // Make sure pages are in order.
    ksort($pages);
    $this->parameters['pages'] = $pages;

    return $grouped;
  }
}

/**
 * NYHS customizations for the scan batch object for William Alexander Pages.
 *
 * @see IslandoraScanBatchObject
 */
class NyhsdoraAlexanderBatchObject extends IslandoraScanBatchObject {
  /**
   * Add collection and content model relationships.
   */
  public function addRelationships() {
    $collection = $this->preprocessorParameters['parent'];
    $predicate = $this->preprocessorParameters['parent_relationship_pred'];
    $uri = $this->preprocessorParameters['parent_relationship_uri'];
    $this->relationships->add($uri, $predicate, $collection);
    $this->addContentModelRelationships();
  }

  /**
   * Add the content model relationship(s).
   */
  protected function addContentModelRelationships() {
    $this->models = array('islandora:bookCModel');
  }

  /**
   * Function batch_process.
   */
  public function batchProcess() {
    $mods = $this->getMods();
    if ($mods) {
      unset($this->label);
      $this->label = $this->getTitle();
    }
    $this->getDc();
    return ISLANDORA_BATCH_STATE__DONE;
  }

  /**
   * Function to get the mods.
   */
  protected function getMods() {
    if (!isset($this['MODS'])) {
      module_load_include('inc', 'rochdora', 'includes/utilities');
      $mods_datastream = $this->constructDatastream('MODS', 'M');
      $mods_datastream->mimetype = 'application/xml';
      $mods_datastream->label = 'MODS Record';
      $mods_file = $this->objectInfo['xml']->uri;
      if (!empty($mods_file)) {
        $mods_datastream->setContentFromFile($mods_file);
      }
      else {
        return FALSE;
      }
      $this->ingestDatastream($mods_datastream);
    }
    return $this['MODS']->content;
  }

  /**
   * Function to get dc.
   */
  protected function getDc() {
    if (!isset($this['DC'])) {
      $dc_datastream = $this->constructDatastream('DC', 'M');
      $dc_datastream->mimetype = 'application/xml';
      $dc_datastream->label = 'DC Record';
      $mods_content = $this->getMods();
      if ($mods_content) {
        $dc_datastream->content = static::runXslTransform(
          array(
            'xsl' => $this->modsToDcTransform,
            'input' => $mods_content,
          )
        );
        $this->ingestDatastream($dc_datastream);
      }
      else {
        return FALSE;
      }
    }
    return $this['DC']->content;
  }

  /**
   * Get the pages to ingest from the mods.
   */
  public function getChildren(IslandoraTuque $connection) {
    $children = array();
    $sequence_number = 1;
    foreach ($this->preprocessorParameters['pages'] as $base_name => $object_info) {
      $children[] = new NyhsdoraAlexanderBatchPageObjects($connection, $this->id, $sequence_number, $object_info, $this->preprocessorParameters, $this->baseName);
      $sequence_number = $sequence_number + 1;
    }
    return $children;
  }
}

/**
 * NYHS customizations for the scan batch object for William Alexander Pages.
 *
 * @see IslandoraScanBatchObject
 */
class NyhsdoraAlexanderBatchPageObjects extends IslandoraScanBatchObject {
  protected $parentId;
  protected $sequenceNumber;
  /**
   * Constructor.
   */
  public function __construct(IslandoraTuque $connection, $parent_id, $sequence, $object_info, $pp) {
    parent::__construct($connection, $parent_id, $object_info, $pp);
    $this->parentId = $parent_id;
    $this->sequenceNumber = $sequence;
    $this->label = $this->objectInfo['tif']->name;
  }

  /**
   * Add collection and content model relationships.
   */
  public function addRelationships() {
    module_load_include('inc', 'islandora_paged_content', 'includes/utilities');
    islandora_paged_content_add_relationships_to_child($this, array(
      'model' => 'islandora:pageCModel',
      'parent' => $this->parentId,
      'page_number' => $this->sequenceNumber,
      'extract_text' => 'none',
    ));
  }

  /**
   * Function batch_process.
   */
  public function batchProcess() {
    $this->getObj();
    return ISLANDORA_BATCH_STATE__DONE;
  }

  /**
   * Function to get the OBJ.
   */
  protected function getObj() {
    if (!isset($this['OBJ'])) {
      list($dsid, $mimetype) = static::determineDSIDAndMimetype($this->objectInfo['tif']->filename);
      $obj_datastream = $this->constructDatastream('OBJ', 'M');
      $obj_datastream->mimetype = $mimetype;
      $obj_datastream->label = $this->objectInfo['tif']->filename;
      $obj_datastream->setContentFromFile($this->objectInfo['tif']->uri, FALSE);

      $this->ingestDatastream($obj_datastream);
    }
  }
}
