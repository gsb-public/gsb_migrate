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

/**
 * Base class for all node migrations - handles commonalities across all
 * supported source Drupal versions.
 *
 * In addition to the arguments supported by DrupalMigration, the following
 * must be passed in the $arguments array:
 *
 * source_type - Drupal 6 content type machine name.
 * destination_type - Drupal 7 content type machine name (bundle).
 *
 * The following optional arguments may be passed:
 *
 * user_migration - Machine name of a user migration, used to establish
 *   dependencies and a sourceMigration for the uid mapping.
 * default_uid - Drupal 7 (destination) uid of the user account to use as
 *   the default.
 * default_language - Default language for the node and node body. Defaults
 *   to LANGUAGE_NONE.
 */
class DrupalProfile2NodeMigration extends DrupalMigration {
  /**
   * The source and destination content types (bundles) we're dealing with.
   */
  protected $destinationType;

  /**
   * Default language to apply to the node and it's body field.
   *
   * @var string
   */
  protected $defaultLanguage = LANGUAGE_NONE;

  /**
   * @param array $arguments
   */
  public function __construct(array $arguments) {
    $this->destinationType = $arguments['destination_type'];
    $this->sourceType = $arguments['source_type'];
    if (!empty($arguments['user_migration'])) {
      $user_migration = $arguments['user_migration'];
      $this->dependencies[] = $user_migration;
    }
    if (!empty($arguments['default_language'])) {
      $this->defaultLanguage = $arguments['default_language'];
    }
    parent::__construct($arguments);

    // Document known core fields
    $this->sourceFields += array(
      'pid' => t('Profile ID'),
      'uid' => t('Authored by (uid)'),
      'created' => t('Created timestamp'),
      'changed' => t('Modified timestamp'),
    );

    $this->sourceFields += $this->version->getSourceFields('profile2', $this->sourceType);

    $this->source = new MigrateSourceSQL($this->query(), $this->sourceFields, NULL,
      $this->sourceOptions);

    $this->destination = new MigrateDestinationNode($this->destinationType);

    $this->map = new MigrateSQLMap($this->machineName,
      array(
        'pid' => array(
          'type' => 'int',
          'unsigned' => TRUE,
          'not null' => TRUE,
          'description' => 'Source Profile PID',
          'alias' => 'p',
        ),
      ),
      MigrateDestinationNode::getKeySchema()
    );

    if (!$this->newOnly) {
      $this->highwaterField = array(
        'name' => 'changed',
        'alias' => 'p',
        'type' => 'int',
      );
    }

    // Setup common mappings
    $this->addSimpleMappings(array('created', 'changed'));

    if (!empty($arguments['default_uid'])) {
      $default_uid = $arguments['default_uid'];
    }
    else {
      $default_uid = 1;
    }

    if (isset($user_migration)) {
      $this->addFieldMapping('uid', 'uid')
        ->sourceMigration($user_migration)
        ->defaultValue($default_uid);
    }
    else {
      $this->addFieldMapping('uid')
        ->defaultValue($default_uid);
    }

    if (module_exists('pathauto')) {
      $this->addFieldMapping('pathauto')
        ->description('By default, disable in favor of migrated paths')
        ->defaultValue(0);
    }
  }

  /**
   * Called after the query data is fetched - we'll use this to populate the
   * source row with the CCK fields.
   */
  public function prepareRow($row) {
    if (parent::prepareRow($row) === FALSE) {
      return FALSE;
    }

    $this->version->getSourceValues($row, $row->pid);
  }

  /**
   * Query for basic profile2 fields from Drupal 7.
   *
   * @return QueryConditionInterface
   */
  protected function query() {
    $query = Database::getConnection('default', $this->sourceConnection)
      ->select('profile', 'p')
      ->fields('p', array('pid','uid', 'created', 'changed'))
      ->condition('p.type', $this->sourceType)
      ->orderBy('p.changed');

    return $query;
  }

  /**
   * Implementation of Migration::createStub().
   *
   * @param $migration
   * @return array|bool
   */
  protected function createStub($migration) {
    migrate_instrument_start('DrupalNodeMigration::createStub');
    $node = new stdClass;
    $node->title = t('Stub');
    $node->type = $this->destination->getBundle();
    $node->uid = 1;
    node_save($node);
    migrate_instrument_stop('DrupalNodeMigration::createStub');
    if (isset($node->nid)) {
      return array($node->nid);
    }
    else {
      return FALSE;
    }
  }
}

class GSBFacultyD2DMigration extends DrupalProfile2NodeMigration {
  public function __construct(array $arguments) {
    parent::__construct($arguments);
  }

  protected function query() {
    $query = parent::query();
    $query->leftjoin('users', 'u', 'p.uid = u.uid');
    $query->addField('u', 'name');
    return $query;
  }
}

class GSBDrupal extends DrupalVersion7 {

  protected function populateSourceFieldInfo($entity_type, $bundle, $include_body = FALSE) {
    parent::populateSourceFieldInfo($entity_type, $bundle, $include_body);

    // Populate SourceFieldInfo with field collection fields.
    $this->populateFieldCollectionSourceFieldInfo();
  }

  public function populateFieldCollectionSourceFieldInfo() {
    foreach ($this->sourceFieldInfo as $field_name => $info) {
      if ($info['type'] == 'field_collection') {
        // Init array with field collection fields.
        $this->sourceFieldInfo[$field_name]['field_collection_fields'] = array();
        // Get each field attached to this type.
        if (Database::getConnection('default', $this->arguments['source_connection'])
          ->schema()->tableExists('field_config_instance')) {
          $query = Database::getConnection('default', $this->arguments['source_connection'])
            ->select('field_config_instance', 'i')
            ->fields('i', array('data'))
            ->condition('entity_type', 'field_collection_item')
            ->condition('bundle', $field_name)
            ->condition('i.deleted', 0);
          $query->innerJoin('field_config', 'f', 'i.field_name = f.field_name');
          $query->fields('f', array('field_name', 'type', 'module'));
          $result = $query->execute();
          foreach ($result as $row) {
            $data = !empty($row->data) ? unserialize($row->data) : array();
            // Although a format column is present for text fields with text
            // filtering disabled, we want to skip it
            if (substr($row->type, 0, 4) == 'text' &&
              $data['settings']['text_processing'] == 0) {
              $skip_format = TRUE;
            }
            else {
              $skip_format = FALSE;
            }
            $columns = $this->getSourceFieldColumns($row->field_name, $skip_format);
            foreach ($columns as $key => $column) {
              $columns['fc_' . $field_name . '_' . $key] = $column;
              unset($columns[$key]);
            }
            $this->sourceFieldInfo[$field_name]['field_collection_fields'][$row->field_name] = $columns;
          }
        }
      }
    }
  }

  public function getSourceValues($row, $entity_id) {
    parent::getSourceValues($row, $entity_id);
    // Load up field data for dynamically mapped fields
    foreach ($this->sourceFieldInfo as $field_name => $info) {
      if ($info['type'] == 'field_collection') {
        foreach ($this->sourceFieldInfo[$field_name]['field_collection_fields'] as $fc_field_name => $columns) {
          // Find the data in field_data_$fc_field_name.
          $table = "field_data_$fc_field_name";
          $result = Database::getConnection('default', $this->arguments['source_connection'])
            ->select($table, 'f')
            ->fields('f')
            ->condition('entity_type', 'field_collection')
            ->condition('bundle', $field_name)
            ->condition('entity_id', $entity_id)
            ->orderBy('delta')
            ->execute();
          foreach ($result as $field_row) {
            foreach ($columns as $display_name => $column_name) {
              if (isset($row->$display_name) && !is_array($row->$display_name)) {
                $row->$display_name = array($row->$display_name);
              }
              $row->{$display_name}[] = $field_row->$column_name;
            }
          }
        }
      }
    }
  }

  public function getSourceFields($entity_type, $bundle, $include_body = FALSE) {
    $fields = parent::getSourceFields($entity_type, $bundle, $include_body);
    foreach ($this->sourceFieldInfo as $field_name => $info) {
      if ($info['type'] == 'field_collection') {
        foreach ($info['field_collection_fields'] as $field_name => $columns) {
          foreach ($columns as $display_name => $column_name) {
            $fields[$display_name] = t('!label subfield', array('!label' => $info['label']));
          }
        }
      }
    }

    return $fields;
  }
}
