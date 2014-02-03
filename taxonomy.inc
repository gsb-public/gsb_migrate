<?php

class GSBTaxonomyAcademicAreaMigration extends DrupalTerm7Migration {
  public function __construct(array $arguments) {
    parent::__construct($arguments);
  }

  /**
   * Review a data row after fetch, returning FALSE to skip it.
   *
   * @param $row
   * @return bool
   */
  public function prepareRow($row) {
    if (parent::prepareRow($row) === FALSE) {
      return FALSE;
    }

    $taxonomy_mapping = array(
      'Operations, Information and Technology' => 'Operations, Information & Technology',
    );

    if (array_key_exists($row->name, $taxonomy_mapping)) {
      $row->name = $taxonomy_mapping[$row->name];
    }

    $vocabulary = taxonomy_vocabulary_machine_name_load($this->arguments['destination_vocabulary']);
    $term = taxonomy_term_load_multiple(NULL, array('name' => $row->name, 'vid' => $vocabulary->vid));

    // Migrate only terms that already exist. This is needed for map table to be generated.
    if (!$term) {
      return FALSE;
    }

    return TRUE;
  }
}

class GSBTaxonomyBusWebsiteCategoriesMigration extends Migration {
  public function __construct(array $arguments) {
    parent::__construct($arguments);

    // Create our three main objects - source, destination, and map
    $this->source = new MigrateSourceCSV($this->arguments['source_file'], $this->csvcolumns());

    $this->destination = new MigrateDestinationTerm($this->arguments['destination_vocabulary']);

    $this->map = new MigrateSQLMap($this->machineName,
      array(
        'term_name' => array(
          'type' => 'varchar',
          'length' => 255,
          'not null' => TRUE,
          'description' => 'Source term',
        ),
      ),
      MigrateDestinationTerm::getKeySchema()
    );

    $this->addFieldMapping('name', 'term_name');
  }

  public function csvcolumns() {
    return array(array('term_name', 'Source term name'));
  }
}
