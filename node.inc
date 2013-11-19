<?php

/**
 * GSBFacultyMigration class.
 *  - Faculty Profiles to Faculty CT migration.
 */
class GSBFacultyMigration extends Migration {
  public function __construct($arguments) {
    parent::__construct($arguments);
    $this->description = t('Migrate Faculty profile2 entity to Faculty CT node.');

    $this->map = new MigrateSQLMap($this->machineName,
      array(
        'pid' => array(
          'type' => 'int',
          'unsigned' => TRUE,
          'not null' => TRUE,
        )
      ),
      MigrateDestinationNode::getKeySchema()
    );

    $query = Database::getConnection('default', $arguments['source_connection'])
      ->select('profile', 'p')
      ->fields('p', array('pid','uid', 'created', 'changed'))
      ->condition('p.type', 'faculty');

    // Add tables and fields to your query to include all of your profile2 fields
    $query->leftjoin('users', 'u', 'p.uid = u.uid');
    $query->addField('u', 'name');
    // @TODO: Join fields tables here.

    $this->source = new MigrateSourceSQL($query);
    $this->destination = new MigrateDestinationNode('faculty');

    // Assign author.
    $this->addFieldMapping('uid')
      ->defaultValue(1);

    // Map created and changed fields.
    $this->addSimpleMappings(array('created', 'changed'));

    $this->addFieldMapping('title')
      ->defaultValue('Test title');
    $this->addFieldMapping('field_first_name', 'name');
  }
}
