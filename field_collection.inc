<?php

/**
 * Class GSBCSVFieldCollectionMigration
 *  - Base class for CSV -> Field Collection migrations.
 */
abstract class GSBCSVFieldCollectionMigration extends Migration {
  public function __construct(array $arguments) {
    parent::__construct($arguments);

    if (!empty($this->arguments['dependencies'])) {
      $this->dependencies = $this->arguments['dependencies'];
    }

    // Create a MigrateSource object, which manages retrieving the input data.
    $this->source = new MigrateSourceCSV($this->arguments['source_file'], $this->csvcolumns());
    $this->map = new MigrateSQLMap($this->machineName,
      array(
        'nid' => array(
          'type' => 'int',
          'not null' => TRUE,
        ),
        'csvrownum' => array(
          'type' => 'int',
          'not null' => TRUE,
        ),
      ),
      MigrateDestinationFieldCollection::getKeySchema()
    );

    $this->destination = new MigrateDestinationFieldCollection(
      $this->arguments['destination_type'],
      array('host_entity_type' => 'node')
    );

    $this->addFieldMapping('host_entity_id', 'nid')
      ->sourceMigration($this->arguments['source_migration']);
  }

  /**
   * Define field names for CSV columns.
   */
  abstract protected function csvcolumns();

  function prepareRow($row) {
    if (parent::prepareRow($row) === FALSE) {
      return FALSE;
    }

    // Left here for debugging.
    print $row->nid . "\n";

    return TRUE;
  }
}

/**
 * Class GSBCSVFieldCollectionSplitterMigration
 *  - Base class for CSV -> Field Collection migrations using GSBCSVItemSplitter to construct source.
 */
abstract class GSBCSVFieldCollectionSplitterMigration extends Migration {
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
      MigrateDestinationFieldCollection::getKeySchema()
    );

    $this->destination = new MigrateDestinationFieldCollection(
      $this->arguments['destination_type'],
      array('host_entity_type' => 'node')
    );

    $this->addFieldMapping('host_entity_id', 'nid')
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

  function prepareRow($row) {
    if (parent::prepareRow($row) === FALSE) {
      return FALSE;
    }

    // Left here for debugging.
    print $row->nid . "\n";

    return TRUE;
  }
}

/**
 * Class GSBRelatedFacultyMigration
 *  - Migrate class for Related Faculty migration.
 */
class GSBRelatedFacultyMigration extends GSBCSVFieldCollectionSplitterMigration {
  public function __construct(array $arguments) {
    parent::__construct($arguments);
    $this->addFieldMapping('field_person_fac_single_ref', 'related_faculty');
    $this->addFieldMapping('field_profile_visibility')
      ->defaultValue('Visible')
      ->callbacks(array('GSBMigrateHelper', 'visibilityTextToId'));
  }

  protected function columnsFromSource() {
    return array('nid', 'related_faculty');
  }

  protected function required_fields() {
    return array('related_faculty');
  }
}

/**
 * Class GSBRelatedContentMigration
 *  - Migrate class for Related Content migration.
 */
class GSBRelatedContentMigration extends GSBCSVFieldCollectionSplitterMigration {
  public function __construct(array $arguments) {
    parent::__construct($arguments);

    $this->addFieldMapping('field_related_other_label', 'related_content_label');
    $this->addFieldMapping('field_related_other_content', 'related_content');
  }

  protected function columnsFromSource() {
    return array('nid', 'related_content_label', 'related_content');
  }


  protected function required_fields() {
    return array('related_content', 'related_content_label');
  }
}
