<?php

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

/**
 * Class GSBFacultyBaseMigration
 *  - GSBFacultyBaseMigration class to be extended for Insert and Update migrations.
 */
class GSBFacultyBaseMigration extends DrupalProfile2NodeMigration {
  public function __construct(array $arguments) {
    // Add some Person fields.
    $this->sourceFields['field_person_first_name'] = 'Person: First name';
    $this->sourceFields['field_person_middle_name'] = 'Person: Middle name';
    $this->sourceFields['field_person_last_name'] = 'Person: Last name';
    $this->sourceFields['field_person_sunetid'] = 'Person: Sunetid';
    $this->sourceFields['field_person_personal_url'] = 'Person: Personal URL';
    $this->sourceFields['field_person_work_phone'] = 'Person: Work Phone';
    $this->sourceFields['field_person_email'] = 'Person: Email';
    $this->sourceFields['field_person_hide_work_phone'] = 'Person: Hide Work Phone';
    $this->sourceFields['field_person_hide_email'] = 'Person: Hide Email';

    // Add Faculty prepared fields.
    $this->sourceFields['field_cv_upload_url'] = 'Faculty: CV Upload URL';
    $this->sourceFields['field_awards_honors_concat'] = 'Faculty: Concatenated awards and honors.';
    parent::__construct($arguments);

    // We use Academic Areas here. So we need the terms migrated first.
    $this->dependencies[] = 'GSBTaxonomyAcademicArea';

    // Personal info.
    $this->addFieldMapping('field_first_name', 'field_person_first_name');
    $this->addFieldMapping('field_middle_name', 'field_person_middle_name');
    $this->addFieldMapping('field_last_name', 'field_person_last_name');
    $this->addFieldMapping('field_sunetid', 'field_person_sunetid');
    $this->addFieldMapping('field_link_website', 'field_person_personal_url');
    $this->addFieldMapping('field_phone', 'field_person_work_phone');
    $this->addFieldMapping('field_email', 'field_person_email');

    $this->addFieldMapping('field_title_position_unlimited', 'field_title_academic')
      ->callbacks('array_decode_entities');
    $this->addFieldMapping('field_rank', 'field_official_rank');
    $this->addFieldMapping('field_academic_area_single', 'field_academic_department')
      ->sourceMigration('GSBTaxonomyAcademicArea');
    $this->addFieldMapping('field_academic_area_single:source_type')
      ->defaultValue('tid');
    $this->addFieldMapping('field_academic_area_unlimited', 'field_other_academic_department')
      ->sourceMigration('GSBTaxonomyAcademicArea');
    $this->addFieldMapping('field_academic_area_unlimited:source_type')
      ->defaultValue('tid');

    $this->addFieldMapping('field_research_statement','field_teaching_research')
      ->callbacks('array_strip_tags');
    $this->addFieldMapping('field_body', 'field_bio');
    $this->addFieldMapping('field_body:format')
      ->defaultValue('gsb_wysiwyg_text');

    // field_cv_upload_url field is prepared in prepareRow().
    $this->addFieldMapping('field_file_single_public', 'field_cv_upload_url');
    $this->addFieldMapping('field_file_single_public:destination_dir')
      ->defaultValue('public://cv');

    // Our own Awards and Honors field.
    $this->addFieldMapping('field_awards_honors', 'field_awards_honors_concat')
      ->callbacks('array_strip_tags');
  }

  protected function query() {
    $query = parent::query();
    $query->leftjoin('users', 'u', 'p.uid = u.uid');
    $query->addField('u', 'name');
    $query->leftjoin('profile', 'personp', "personp.uid = u.uid AND personp.type = 'person'");

    $query->leftjoin('field_data_field_sunetid', 'fsid', 'personp.pid = fsid.entity_id');
    $query->addField('fsid', 'field_sunetid_value', 'field_person_sunetid');

    $query->leftjoin('field_data_field_personal_url', 'fpu', 'personp.pid = fpu.entity_id');
    $query->addField('fpu', 'field_personal_url_url', 'field_person_personal_url');

    $query->leftjoin('field_data_field_work_phone', 'fwp', 'personp.pid = fwp.entity_id');
    $query->addField('fwp', 'field_work_phone_value', 'field_person_work_phone');

    $query->leftjoin('field_data_field_hide_phone', 'fhp', 'personp.pid = fhp.entity_id');
    $query->addField('fhp', 'field_hide_phone_value', 'field_person_hide_work_phone');

    $query->leftjoin('field_data_field_person_email', 'fpe', 'personp.pid = fpe.entity_id');
    $query->addField('fpe', 'field_person_email_email', 'field_person_email');

    $query->leftjoin('field_data_field_hide_email', 'fhe', 'personp.pid = fhe.entity_id');
    $query->addField('fhe', 'field_hide_email_value', 'field_person_hide_email');

    $query->leftjoin('field_data_field_name_set', 'fns', 'personp.pid = fns.entity_id');
    $query->leftjoin('field_data_field_person_first_name', 'fnsfn', 'fns.field_name_set_value = fnsfn.entity_id');
    $query->addField('fnsfn', 'field_person_first_name_value', 'field_person_first_name');
    $query->leftjoin('field_data_field_person_last_name', 'fnsln', 'fns.field_name_set_value = fnsln.entity_id');
    $query->addField('fnsln', 'field_person_last_name_value', 'field_person_last_name');
    $query->leftjoin('field_data_field_person_middle_name', 'fnsmn', 'fns.field_name_set_value = fnsmn.entity_id');
    $query->addField('fnsmn', 'field_person_middle_name_value', 'field_person_middle_name');

    return $query;
  }

  public function prepareRow($row) {
    if (parent::prepareRow($row) === FALSE) {
      return FALSE;
    }

    // Skip profile2 entities that don't have sunetid.
    if (empty($row->field_person_sunetid)) {
      return FALSE;
    }

    // Left here for debugging.
    print $row->pid . "\n";

    // Replace person name when override is specified.
    $name_override_field_mapping = array(
      'field_person_first_name' => 'fc_field_name_over_ride_field_first_name:value',
      'field_person_middle_name' => 'fc_field_name_over_ride_field_middle_name:value',
      'field_person_last_name' => 'fc_field_name_over_ride_field_last_name:value',
    );

    // Don't migrate email and phone if those were hidden in legacy site.
    if ($row->field_person_hide_email) {
      unset($row->field_person_email);
    }
    if ($row->field_person_hide_work_phone) {
      unset($row->field_person_work_phone);
    }

    foreach ($name_override_field_mapping as $name_original_field => $name_override_field) {
      if (!empty($row->{$name_override_field}[0][0])) {
        if ($row->{$name_override_field}[0][0] != $row->{$name_original_field}) {
          watchdog($this->machineName, t('Fieldname !fieldname was replaced from !value1 to !value2 for !entity_id',
            array(
              '!fieldname' => $name_original_field,
              '!value1' => $row->{$name_original_field},
              '!value2' => $row->{$name_override_field}[0][0],
              '!entity_id' => $row->pid,
            )));
          $row->{$name_original_field} = $row->{$name_override_field}[0][0];
        }
      }
    }

    // Prepare academic titles.
    $academic_title_fields = array('field_title_academic', 'field_title_executive_education', 'field_title_other');
    $acad_titles = array();
    foreach ($academic_title_fields as $academic_title_field) {
      if (isset($row->{$academic_title_field})) {
        $row->{$academic_title_field} = trim(strip_tags($row->{$academic_title_field}[0]));
        if (!empty($row->{$academic_title_field})) {
          $acad_titles[] = $row->{$academic_title_field};
        }
      }
    }
    $row->field_title_academic = $acad_titles;

    // Handle CV external and CV Upload fields.
    if (!empty($row->field_cv_upload)) {
      $cv_file_uri = Database::getConnection('default', $this->sourceConnection)
        ->select('file_managed', 'f')
        ->condition('fid', $row->field_cv_upload[0])
        ->fields('f', array('uri'))
        ->execute()
        ->fetchField();
    }

    if ($cv_file_uri) {
      $cv_file_uri = str_replace('public://', 'http://gsb.stanford.edu/sites/default/files/', $cv_file_uri);
    }
    else {
      // Use external PDF link in case a local file is not available.
      $cv_file_uri = $row->field_cv_external[0];
    }

    if (!empty($cv_file_uri)) {
      $cv_ext = pathinfo($cv_file_uri, PATHINFO_EXTENSION);
      // Import only PDF CVs.
      if (!empty($cv_ext) && strtolower($cv_ext) == 'pdf') {
        $row->field_cv_upload_url = $cv_file_uri;
      }
    }

    // Concatenate awards and honors.
    $row->field_awards_honors_concat = array();
    if (!empty($row->{"fc_field_awards_honors_field_award_name:value"})) {
      foreach ($row->{"fc_field_awards_honors_field_award_name:value"} as $delta => $award_name)  {
        $award_output = array(
          $row->{"fc_field_awards_honors_field_award_name:value"}[$delta][0],
          $row->{"fc_field_awards_honors_field_award_date:value"}[$delta][0],
          $row->{"fc_field_awards_honors_field_awards_organization:value"}[$delta][0],
        );
        $row->field_awards_honors_concat[] = implode(', ', $award_output);
      }
    }
  }

  /**
   * Fetch existing Faculty nid by sunetid.
   * @param $sunetid
   * @return $destination_id
   */
  public function getFacultyNidBySunetid($sunetid) {
    $destination_id = db_select('field_data_field_sunetid', 'fs')
      ->condition('field_sunetid_value', $sunetid)
      ->condition('bundle', 'faculty')
      ->fields('fs', array('entity_id'))
      ->execute()
      ->fetchField();

    return $destination_id;
  }
}

/**
 * Class GSBFacultyUpdateMigration
 *  - Update manually created Faculty nodes.
 */
class GSBFacultyUpdateMigration extends GSBFacultyBaseMigration {
  public function __construct(array $arguments) {
    $this->sourceFields['destination_id'] = 'Destination ID to be used to merge nodes.';
    parent::__construct($arguments);

    // Specify destination as update source.
    $this->systemOfRecord = Migration::DESTINATION;
    $this->addFieldMapping('nid', 'destination_id');
  }

  public function prepareRow($row) {
    if (parent::prepareRow($row) === FALSE) {
      return FALSE;
    }
    // Try to map existing content by sunetid.
    if (!empty($row->field_person_sunetid)) {
      $destination_id = $this->getFacultyNidBySunetid($row->field_person_sunetid);
      if ($destination_id) {
        $row->destination_id = $destination_id;
        return TRUE;
      }
    }

    return FALSE;
  }
}

/**
 * Class GSBFacultyInsertMigration
 *  - Migrate (insert) Faculty nodes that don't yet exist.
 *  We check sunetid to verify if that's an existing node.
 */
class GSBFacultyInsertMigration extends GSBFacultyBaseMigration {
  public function __construct(array $arguments) {
    parent::__construct($arguments);

    // Let's update existing nodes first.
    $this->dependencies[] = 'GSBFacultyUpdate';

    // Set all newly migrated Faculty profiles to published.
    $this->addFieldMapping('status')
      ->defaultValue(TRUE);
  }

  public function prepareRow($row) {
    if (parent::prepareRow($row) === FALSE) {
      return FALSE;
    }

    if (!empty($row->field_person_sunetid)) {
      $destination_id = $this->getFacultyNidBySunetid($row->field_person_sunetid);
      if (!$destination_id) {
        return TRUE;
      }
    }

    return FALSE;
  }
}
