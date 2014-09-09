<?php

/**
 * Abstract class for Resources migrations from CSV
 * using GSBCSVItemSplitter to construct source.
 */
abstract class GSBResourceMigration extends Migration {
  public function __construct(array $arguments) {
    parent::__construct($arguments);

    if (!empty($this->arguments['dependencies'])) {
      $this->dependencies = $this->arguments['dependencies'];
    }

    $options = array();
    if (method_exists($this, 'required_fields')) {
      $options['required_fields'] = $this->required_fields();
    }

    $splitter = new GSBCSVItemSplitter($this->arguments['source_file'], $this->csvcolumns(), $options);
    $array = $splitter->fetch();
    // Create a MigrateSource object, which manages retrieving the input data.
    $this->source = new MigrateSourceList(
      new GSBListArray($array),
      new GSBItemArray($array)
    );
    $this->map = new MigrateSQLMap($this->machineName,
      array(
        'id' => array(
          'type' => 'int',
          'not null' => TRUE,
        ),
      ),
      MigrateDestinationFile::getKeySchema()
    );

    $this->destination = new MigrateDestinationFile($this->arguments['destination_type'], 'ResourcesResourcesFile');
    $this->addFieldMapping('resource_type')->defaultValue($this->arguments['resource_type']);
    $this->addFieldMapping('resource_host_node', 'nid')
      ->sourceMigration($this->arguments['source_migration']);
  }

  /**
   * Define field names for CSV columns.
   *  - By default this class will search for columns in Source migration if columnsFromSource method is available.
   */
  protected function csvcolumns() {
    $csvcolumns = array();

    if (isset($this->arguments['source_migration']) && method_exists($this, 'columnsFromSource')) {
      $columns = $this->columnsFromSource();
      $migration = Migration::getInstance($this->arguments['source_migration']);
      $csvcolumns = $migration->csvcolumns();

      foreach ($csvcolumns as $key => $column) {
        list($machine_name, $label) = $column;
        if (!in_array($machine_name, $columns)) {
          unset($csvcolumns[$key]);
        }
      }
    }

    return $csvcolumns;
  }

  /**
   * Allow derived classes to implement this method in order to get columns from Source migration.
   *   - It's commented out because we don't force it. Derived classes can still override csvcolumns() method and don't use
   * this method.
   */
  //abstract protected function columnsFromSource();

  /**
   * Allow Migration classes to specify required fields to filter out Splitter results.
   * @return array
   */
  protected function required_fields() {
    return array();
  }

  function prepareRow($row) {
    if (parent::prepareRow($row) === FALSE) {
      return FALSE;
    }

    // Left here for debugging.
    print 'Nid:' . $row->nid . ' ItemID: ' . $row->id . "\n";

    return TRUE;
  }
}


/**
 * Class GSBResourceImageMigration
 *  - Migrate class for Resource Image migration.
 */
class GSBResourceImageMigration extends GSBResourceMigration {
  public function __construct(array $arguments) {
    $arguments['destination_type'] = 'resource_image';
    $arguments['resource_type'] = 'resource_image';
    parent::__construct($arguments);

    // Override MigrateFile class here.
    $this->destination = new MigrateDestinationFile($this->arguments['destination_type'], 'MigrateFileUri');
    $this->addFieldMapping('value', 'resource_image_file');
    // URLs are already encoded.
    $this->addFieldMapping('urlencode')
      ->defaultValue(FALSE);
    $this->addFieldMapping('destination_dir')
      ->defaultValue('public://resources');
    // @todo: Add filename and type to available destination fields. Or consider resource_type field.
    $this->addFieldMapping('filename', 'resource_image_name');
    $this->addFieldMapping('field_body', 'resource_image_caption');
    $this->addFieldMapping('field_file_image_alt_text', 'resource_image_alt');
  }

  protected function columnsFromSource() {
    return array('nid', 'resource_image_name', 'resource_image_file', 'resource_image_caption', 'resource_image_alt');
  }

  protected function required_fields() {
    return array('resource_image_name', 'resource_image_file');
  }

  function prepareRow($row) {
    if (parent::prepareRow($row) === FALSE) {
      return FALSE;
    }

    // Left here for debugging.
    if (!empty($row->resource_image_file) && strpos($row->resource_image_file, 'imgur.com') !== FALSE) {
      // URLs in CSV are of http://imgur.com/JYDJnfY format. We'll just dirty-convert them to publicly downloadable http://i.imgur.com/JYDJnfY.jpg
      $row->resource_image_file = str_replace('imgur.com', 'i.imgur.com', $row->resource_image_file) . '.jpg';
    }

    return TRUE;
  }
}

/**
 * Class GSBResourceYoutubeMigration
 *  - Migrate class for Resource Youtube migration.
 */
class GSBResourceYoutubeMigration extends GSBResourceMigration {
  public function __construct(array $arguments) {
    $arguments['destination_type'] = 'resource_youtube';
    $arguments['resource_type'] = 'resource_youtube';
    parent::__construct($arguments);

    //$this->addFieldMapping('value', 'resource_video_url');
    $this->addFieldMapping('field_youtube_embed_code', 'resource_video_url');
    $this->addFieldMapping('filename', 'resource_video_name');
  }

  protected function columnsFromSource() {
    return array('nid', 'resource_video_name', 'resource_video_url');
  }


  protected function required_fields() {
    return array('resource_video_name', 'resource_video_url');
  }
}

/**
 * Class GSBResourceTwitterMigration
 *  - Migrate class for Resource Twitter migration.
 */
class GSBResourceTwitterMigration extends GSBResourceMigration {
  public function __construct(array $arguments) {
    $arguments['destination_type'] = 'resource_twitter';
    $arguments['resource_type'] = 'resource_twitter';
    parent::__construct($arguments);

    $this->addFieldMapping('field_twitter_url', 'resource_twitter_url');
    $this->addFieldMapping('filename', 'resource_twitter_name');
  }

  protected function columnsFromSource() {
    return array('nid', 'resource_twitter_name', 'resource_twitter_url');
  }

  protected function required_fields() {
    return array('resource_twitter_name', 'resource_twitter_url');
  }
}

/**
 * Class GSBResourceCalloutMigration
 *  - Migrate class for Resource Callout migration.
 */
class GSBResourceCalloutMigration extends GSBResourceMigration {
  public function __construct(array $arguments) {
    $arguments['destination_type'] = 'resource_pull_quote';
    $arguments['resource_type'] = 'resource_callout';
    parent::__construct($arguments);

    $this->addFieldMapping('filename', 'resource_callout_name');
    $this->addFieldMapping('field_description', 'resource_callout_text');
    $this->addFieldMapping('field_quote_author', 'resource_callout_attribution');
  }

  protected function columnsFromSource() {
    return array('nid', 'resource_callout_name', 'resource_callout_text', 'resource_callout_attribution');
  }

  protected function required_fields() {
    return array('resource_callout_name', 'resource_callout_text');
  }
}
