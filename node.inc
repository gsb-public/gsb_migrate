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
      if (isset($row->{$academic_title_field}[0])) {
        // Handle multiple lines as separate titles.
        $title_rows = explode("\n", $row->{$academic_title_field}[0]);
        foreach ($title_rows as $title_row) {
          $title_row = trim(strip_tags($title_row));
          if (!empty($title_row)) {
            $acad_titles[] = $title_row;
          }
        }
      }
    }
    $row->field_title_academic = $acad_titles;

    watchdog('gsb_migrate_cv_debug', "Start handling CV field for %pid", array('%pid' => $row->pid));
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
      watchdog('gsb_migrate_cv_debug', "CV Upload field has value: %cv_uri.", array('%cv_uri' => $cv_file_uri));
      $cv_file_uri = str_replace('public://', 'http://www.gsb.stanford.edu/sites/default/files/', $cv_file_uri);
      watchdog('gsb_migrate_cv_debug', "CV Upload field URL is modified to: %cv_uri.", array('%cv_uri' => $cv_file_uri));
    }
    else {
      // Use external PDF link in case a local file is not available.
      $cv_file_uri = $row->field_cv_external[0];
      watchdog('gsb_migrate_cv_debug', "CV External field is used with value: %cv_uri.", array('%cv_uri' => $cv_file_uri));
    }

    if (!empty($cv_file_uri)) {
      $cv_ext = pathinfo($cv_file_uri, PATHINFO_EXTENSION);
      // Import only PDF CVs.
      if (!empty($cv_ext) && strtolower($cv_ext) == 'pdf') {
        $row->field_cv_upload_url = $cv_file_uri;
        watchdog('gsb_migrate_cv_debug', "CV final mapped value is: %cv_uri.", array('%cv_uri' => $cv_file_uri));
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

/**
 * Class GSBBookMigration
 *  - Migrate class for Book CT migration.
 *    Book is considered legacy 'publication' content type with field_publication_type == BOOK.
 */
class GSBBookMigration extends DrupalNode7Migration {
  public function __construct(array $arguments) {
    parent::__construct($arguments);

    $this->addFieldMapping('field_year_of_publication', 'field_year_published');
    $this->addFieldMapping('field_link_unlimited', 'field_publication_url');

    // Override title mapped in parent class.
    $this->removeFieldMapping('title');
    $this->addFieldMapping('title', 'title')
      ->callbacks('strip_tags');
    $this->addFieldMapping('field_description', 'body');
    $this->addFieldMapping('field_description:format')
      ->defaultValue('gsb_wysiwyg_text');

    $this->addSimpleMappings(array('field_publisher'));

  }

  public function query() {
    $query = parent::query();
    $query->leftjoin('field_data_field_publication_type', 'fpt', 'n.nid = fpt.entity_id');
    $query->condition('fpt.field_publication_type_value','BOOK');

    return $query;
  }

  public function prepareRow($row) {
    if (parent::prepareRow($row) === FALSE) {
      return FALSE;
    }

    // Left here for debugging.
    print $row->nid . "\n";

    if (!empty($row->field_year_published[0])) {
      // Convert year to a format that date module can digest.
      if ($date = date_create_from_format('Y-m-d H:i:s', $row->field_year_published[0] . '-01-01 00:00:00')) {
         $row->field_year_published[0] = date_format($date, 'Y-m-d H:i:s');
      }
    }
  }
}

/**
 * Class GSBBookAuthorsMigration
 *  - Migrate Book authors in field collection field.
 */
class GSBBookAuthorsMigration extends DrupalMigration {
  public function __construct(array $arguments) {
    parent::__construct($arguments);
    $this->dependencies = array('GSBBook', 'GSBFacultyInsert', 'GSBFacultyUpdate');

    // Create a MigrateSource object, which manages retrieving the input data.
    $this->source = new MigrateSourceSQL($this->query(), $this->sourceFields, NULL,
      $this->sourceOptions);
    $this->map = new MigrateSQLMap($this->machineName,
      array(
        'nid' => array(
          'type' => 'int',
          'not null' => true,
        ),
        'profile_pid' => array(
          'type' => 'int',
          'not null' => true,
        ),
      ),
      MigrateDestinationFieldCollection::getKeySchema()
    );

    $this->destination = new MigrateDestinationFieldCollection(
      'field_authors',
      array('host_entity_type' => 'node')
    );

    $this->addFieldMapping('host_entity_id', 'nid')
      ->sourceMigration('GSBBook');

    $this->addFieldMapping('field_person_fac_or_other')
      ->defaultValue('UseEntityReference');
    $this->addFieldMapping('field_person_fac_single_ref', 'profile_pid')
      ->sourceMigration(array('GSBFacultyInsert', 'GSBFacultyUpdate'));
  }

  function query() {
    $query = Database::getConnection('default', $this->sourceConnection)
      ->select('node', 'n');
    $query->join('field_data_field_publication_type', 'fpt', 'n.nid = fpt.entity_id');
    $query->join('field_data_field_publication_name', 'fpn', 'fpn.field_publication_name_target_id = n.nid');
    $query->join('field_data_field_publication', 'fp', 'fp.field_publication_value = fpn.entity_id');
    $query->addField('fp', 'entity_id', 'profile_pid');
    $query->fields('n', array('nid'))
      ->condition('fpt.field_publication_type_value','BOOK')
      ->condition('type', 'publication')
      ->orderBy('nid')
      ->orderBy('fp.entity_id');

    return $query;
  }

  function prepareRow($row) {
    if (parent::prepareRow($row) === FALSE) {
      return FALSE;
    }

    // Left here for debugging.
    print $row->nid . "\n";

    // Migrate only if faculty author was migrated and is available.
    $destination_nid = $this->handleSourceMigration(array('GSBFacultyInsert', 'GSBFacultyUpdate'), $row->profile_pid);
    if (empty($destination_nid)) {
      $this->queueMessage(t('Faculty author with legacy pid %profile_pid is not available. Skipped.',
        array('%profile_pid' => $row->profile_pid)));
      return FALSE;
    }

    return TRUE;
  }
}

/**
 * Class GSBPublicationMigration
 *  - Migrate class for Publication CT migration.
 */
class GSBPublicationMigration extends Migration {
  /**
   * Path for authors file generated during publications import.
   */
  public $filename;

  public function __construct(array $arguments) {
    parent::__construct($arguments);

    // Set filename for publication-authors.csv file.
    $this->filename = drupal_get_path('module', 'gsb_migrate') . '/data/publication-authors.csv';

    // Create a MigrateSource object, which manages retrieving the input data.
    $this->source = new MigrateSourceCSV($arguments['source_file'], $this->csvcolumns(), array(), $this->fields());
    $this->map = new MigrateSQLMap($this->machineName,
      array(
        'nid' => array(
          'type' => 'int',
          'not null' => true,
        ),
      ),
      MigrateDestinationNode::getKeySchema()
    );

    $this->destination = new MigrateDestinationNode('publication');

    $this->addSimpleMappings(array('title', 'created', 'status'));
    $this->addFieldMapping('uid')
      ->defaultValue(1);
    //$this->addFieldMapping('field_authors', 'authors');
    $this->addFieldMapping('field_publisher', 'publisher');
    $this->addFieldMapping('field_place_of_publication', 'place_of_publication');
    $this->addFieldMapping('field_description', 'abstract');
    $this->addFieldMapping('field_pub_book_title', 'pub_book_title');
    $this->addFieldMapping('field_pub_date_day', 'publication_day');
    $this->addFieldMapping('field_pub_date_month', 'publication_month');
    $this->addFieldMapping('field_year_of_publication', 'publication_year');
    $this->addFieldMapping('field_volume_number', 'volume');
    $this->addFieldMapping('field_issue_number', 'issue');
    $this->addFieldMapping('field_page_count', 'pages');
    $this->addFieldMapping('field_link_single', 'link_to_source');
  }

  /**
   * Define field names for CSV columns.
   */
  protected function csvcolumns() {
    $columns[2]  = array('created', 'Created Date');
    $columns[3]  = array('status', 'Status');
    $columns[4]  = array('nid', 'Nid');
    $columns[7]  = array('author_sunetid', 'Author SUNETID');
    $columns[8]  = array('title', 'Title');
    $columns[9]  = array('publisher', 'Publisher');
    $columns[10] = array('link_to_source', 'Link to Source');
    $columns[11] = array('publication_day', 'Publication Day');
    $columns[12] = array('publication_month', 'Publication Month');
    $columns[13] = array('publication_year', 'Publication Year');
    $columns[15] = array('abstract', 'Abstract');
    $columns[16] = array('pub_book_title', 'Source Publication / Book Title');
    $columns[17] = array('place_of_publication', 'Place of Publication');
    $columns[18] = array('volume', 'Volume #');
    $columns[19] = array('issue', 'Issue #');
    $columns[20] = array('pages', 'Pages #');
    $columns[22] = array('author_first_name', 'Non GSB Author First Name');
    $columns[23] = array('author_last_name', 'Non GSB Author Last Name');

    return $columns;
  }

  protected function fields() {
    $fields = array();

    $fields['authors'] = 'Computed authors field prepared in prepareRow()';

    return $fields;
  }

  function prepareRow($row) {
    if (parent::prepareRow($row) === FALSE) {
      return FALSE;
    }

    // Left here for debugging.
    print $row->nid . "\n";

    if (!empty($row->publication_year)) {
      // Convert year to a format that date module can digest.
      if ($converted_date = DateTime::createFromFormat('Y-m-d H:i:s', $row->publication_year . '-01-01 00:00:00')) {
        $row->publication_year = $converted_date->format('Y-m-d H:i:s');
      }
      else {
        $this->queueMessage(t('Publication Year is not a valid year. Value: %value',
          array('%value' => $row->publication_year)));
        unset($row->publication_year);
      }
    }

    if (!empty($row->publication_month) && !is_numeric($row->publication_month)) {
      $this->queueMessage(t('Publication Month should be a valid integer. Value: %value',
        array('%value' => $row->publication_month)));
      unset($row->publication_month);
    }

    if (!empty($row->publication_day) && !is_numeric($row->publication_day)) {
      $this->queueMessage(t('Publication Day should be a valid integer. Value: %value',
        array('%value' => $row->publication_day)));
      unset($row->publication_day);
    }

    return TRUE;
  }

  function complete($entity, stdClass $row) {
    // Do nothing when authors.csv is already built. This code is left here to rebuild authors.csv when changes will appear.
    return;

    $lines = array();
    $line = array(
      'nid' => $row->nid,
      'author_sunetid' => '',
      'author_first_name' => '',
      'author_last_name' => '',
    );

    if (!empty($row->author_sunetid)) {
      $authors_sunetid = explode('|', $row->author_sunetid);
      foreach ($authors_sunetid as $author_sunetid) {
        $lines[] = array_merge($line, array('author_sunetid' => $author_sunetid));
      }
    }

    if (!empty($row->author_first_name) && !empty($row->author_last_name)) {
      $authors_first_names = explode('|', $row->author_first_name);
      $authors_last_names = explode('|', $row->author_last_name);
      if (count($authors_first_names) == count($authors_last_names)) {
        foreach ($authors_first_names as $key => $author_first_name) {
          if ($author_first_name != 'multiple authors') {
            $lines[] = array_merge($line, array(
              'author_first_name' => $authors_first_names[$key],
              'author_last_name' =>  $authors_last_names[$key],
            ));
          }
        }
      }
    }

    // Write CSV file.
    foreach ($lines as $line) {
      $line = implode(',', $line) . "\n";
      file_put_contents($this->filename, $line, FILE_APPEND | LOCK_EX);
    }
  }
}

class GSBPublicationAuthorsMigration extends Migration {
  public function __construct(array $arguments) {
    parent::__construct($arguments);
    $this->dependencies = array('GSBPublication', 'GSBFacultyInsert', 'GSBFacultyUpdate');

    // Create a MigrateSource object, which manages retrieving the input data.
    $this->source = new MigrateSourceCSV($arguments['source_file'], $this->csvcolumns(), array(), $this->fields());
    $this->map = new MigrateSQLMap($this->machineName,
      array(
        'nid' => array(
          'type' => 'int',
          'not null' => TRUE,
        ),
        'faculty_nid' => array(
          'type' => 'int',
        ),
        'author_first_name' => array(
          'type' => 'varchar',
          'length' => 255,
          'default' => '',
        ),
        'author_last_name' => array(
          'type' => 'varchar',
          'length' => 255,
          'default' => '',
        ),
      ),
      MigrateDestinationFieldCollection::getKeySchema()
    );

    $this->destination = new MigrateDestinationFieldCollection(
      'field_authors',
      array('host_entity_type' => 'node')
    );

    $this->addFieldMapping('host_entity_id', 'nid')
      ->sourceMigration('GSBPublication');

    $this->addFieldMapping('field_person_fac_or_other', 'fac_or_other');
    $this->addFieldMapping('field_person_fac_single_ref', 'faculty_nid');
    $this->addFieldMapping('field_first_name', 'author_first_name');
    $this->addFieldMapping('field_last_name', 'author_last_name');
  }

  /**
   * Define additional computed fields.
   * @return array
   */
  protected function fields() {
    $fields = array();

    $fields['fac_or_other'] = 'Field switcher for Authors field collection. Possible values: UseEntityReference, Other';
    $fields['faculty_nid'] = 'Faculty NID looked up by sunetid in prepareRow().';

    return $fields;
  }

  /**
   * Define field names for CSV columns.
   */
  protected function csvcolumns() {
    $columns[0] = array('nid', 'Nid');
    $columns[1] = array('author_sunetid', 'Author SUNETID');
    $columns[2] = array('author_first_name', 'Non GSB Author First Name');
    $columns[3] = array('author_last_name', 'Non GSB Author Last Name');

    return $columns;
  }

  function prepareRow($row) {
    if (parent::prepareRow($row) === FALSE) {
      return FALSE;
    }

    // Left here for debugging.
    print $row->nid . "\n";

    if (!empty($row->author_sunetid)) {
      // Migrate only if faculty author was migrated and is available.
      $query = new EntityFieldQuery();
      $query->entityCondition('entity_type', 'node')
        ->entityCondition('bundle', 'faculty')
        ->fieldCondition('field_sunetid', 'value', $row->author_sunetid);
      $result = $query->execute();
      if (isset($result['node'])) {
        $faculty_nid = array_keys($result['node']);

        // We've found a faculty author and we set fac_or_other to use entity reference.
        $row->faculty_nid = $faculty_nid;
        $row->fac_or_other = 'UseEntityReference';
      }
      else {
        $this->queueMessage(t('Faculty author with sunetid %sunetid is not available. Skipped.',
          array('%sunetid' => $row->author_sunetid)));
        return FALSE;
      }
    }

    // Set fac_or_other to Other in case we have simple First Name and Last Name.
    if (!empty($row->author_first_name) && !empty($row->author_last_name)) {
      $row->fac_or_other = 'Other';
    }

    // Ensure we have a value for faculty_nid.
    if (empty($row->faculty_nid)) {
      $row->faculty_nid = 0;
    }

    return TRUE;
  }
}

/**
 * Class GSBBusinessWebsiteMigration
 *  - Migrate class for Business Website CT migration.
 */
class GSBBusinessWebsiteMigration extends Migration {
  public function __construct(array $arguments) {
    parent::__construct($arguments);
    $this->dependencies = array('GSBTaxonomyBusWebsiteCategories');
    // Create a MigrateSource object, which manages retrieving the input data.
    $this->source = new MigrateSourceCSV($arguments['source_file'], $this->csvcolumns());
    $this->map = new MigrateSQLMap($this->machineName,
      array(
        'csvrownum' => array(
          'type' => 'int',
          'not null' => true,
        ),
      ),
      MigrateDestinationNode::getKeySchema()
    );

    $this->destination = new MigrateDestinationNode('business_website');

    $this->addSimpleMappings(array('title'));
    $this->addFieldMapping('field_link_single', 'link');
    $this->addFieldMapping('field_bus_website_description', 'description');
    $this->addFieldMapping('field_bus_website_categories', 'category')
      ->separator(':');
    $this->addFieldMapping('uid')
      ->defaultValue(1);
    $this->addFieldMapping('status')
      ->defaultValue(TRUE);
  }

  /**
   * Define field names for CSV columns.
   */
  protected function csvcolumns() {
    $columns[0]  = array('category', 'Category');
    $columns[1]  = array('title', 'Title');
    $columns[2]  = array('description', 'Description');
    $columns[3]  = array('link', 'Link to website');
    return $columns;
  }

  function prepareRow($row) {
    if (parent::prepareRow($row) === FALSE) {
      return FALSE;
    }

    // Left here for debugging.
    print $row->csvrownum . '. ' . $row->title . "\n";

    return TRUE;
  }
}

class GSBMediaItemMigration extends Migration {
  public function __construct(array $arguments) {
    parent::__construct($arguments);

    // Create a MigrateSource object, which manages retrieving the input data.
    $this->source = new MigrateSourceCSV($arguments['source_file'], $this->csvcolumns());
    $this->map = new MigrateSQLMap($this->machineName,
      array(
        'nid' => array(
          'type' => 'int',
          'not null' => true,
        ),
      ),
      MigrateDestinationNode::getKeySchema()
    );

    $this->destination = new MigrateDestinationNode('media_item');

    $this->addSimpleMappings(array('title'));
    $this->addFieldMapping('uid')
      ->defaultValue(1);
    $this->addFieldMapping('status', 'status')
      ->callbacks('gsb_string_to_bool');
    $this->addFieldMapping('field_editorial_summary', 'teaser');
    $this->addFieldMapping('field_link_single', 'link_to_article');
    $this->addFieldMapping('field_person_fac_single_ref', 'faculty');
    $this->addFieldMapping('field_profile_visibility')
      ->defaultValue(TRUE);
    $this->addFieldMapping('field_pub_book_title', 'source_publication');
    $this->addFieldMapping('field_date', 'publication_date');
  }

  /**
   * Define field names for CSV columns.
   */
  protected function csvcolumns() {
    $columns[0]  = array('nid', 'Newsmaker NID');
    $columns[1]  = array('title', 'Title');
    $columns[2]  = array('uid', 'Author uid');
    $columns[3]  = array('name', 'Author name');
    $columns[4]  = array('publication_date', 'Publication date');
    $columns[5]  = array('created', 'Created date');
    $columns[6]  = array('updated', 'Updated date');
    $columns[7]  = array('content_image', 'Content Image available');
    $columns[8]  = array('status', 'Status');
    $columns[9]  = array('source_publication', 'Source publication');
    $columns[10] = array('faculty', 'Faculty authors');
    $columns[11] = array('function', 'Function (Taxonomy)');
    $columns[12] = array('company', 'Company (Taxonomy)');
    $columns[13] = array('principle', 'Principle (Taxonomy)');
    $columns[14] = array('industry', 'Industry (Taxonomy)');
    $columns[15] = array('operational_topics', 'Operational Topics (Taxonomy)');
    $columns[16] = array('region', 'Region (Taxonomy)');
    $columns[17] = array('sponsors', 'Operational Topics (Taxonomy)');
    $columns[18] = array('tag', 'Tag (Taxonomy)');
    $columns[19] = array('link_to_article', 'Link to Article');
    $columns[20] = array('teaser', 'Teaser');

    return $columns;
  }

  function prepareRow($row) {
    if (parent::prepareRow($row) === FALSE) {
      return FALSE;
    }

    // Left here for debugging.
    print $row->csvrownum . '. ' . $row->title . "\n";

    $row->publication_date = date('Y-m-d H:i:s', (int) $row->publication_date);
    $faculty_authors = explode(', ', $row->faculty);
    if (!empty($faculty_authors)) {
      $sunetid = $faculty_authors[0];
      $faculty_nid = $this->getFacultyNidBySunetid($sunetid);
      if (is_numeric($faculty_nid)) {
        $row->faculty = $faculty_nid;
      }
    }

    return TRUE;
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
 * Class GSBWorkingPaperOtherAuthorsMigration
 *  - Migrate class for Working Paper CT Other Authors migration.
 */
class GSBWorkingPaperOtherAuthorsMigration extends Migration {
  
  public function __construct(array $arguments) {

    parent::__construct($arguments);

    // Create a MigrateSource object, which manages retrieving the input data.
    $this->source = new MigrateSourceCSV($arguments['source_file'], $this->csvcolumns(), array(), $this->fields());

    $this->map = new MigrateSQLMap($this->machineName,
      array(
        'nid' => array(
          'type' => 'int',
          'not null' => TRUE,
        ),
        'author_first_name' => array(
          'type' => 'varchar',
          'length' => 255,
          'default' => '',
        ),
        'author_last_name' => array(
          'type' => 'varchar',
          'length' => 255,
          'default' => '',
        ),
      ),
      MigrateDestinationFieldCollection::getKeySchema()
    );

    $this->destination = new MigrateDestinationFieldCollection(
      'field_authors',
      array('host_entity_type' => 'node')
    );

    $this->addFieldMapping('host_entity_id', 'nid');

    $this->addFieldMapping('field_person_fac_single_ref', 'faculty_nid');
    $this->addFieldMapping('field_first_name', 'author_first_name');
    $this->addFieldMapping('field_last_name', 'author_last_name');

    $this->addFieldMapping('field_person_fac_or_other', 'fac_or_other');

  }

  /**
   * Define field names for CSV columns.
   */
  protected function csvcolumns() {
    $columns[0] = array('nid', 'Node Id');
    $columns[1] = array('author_first_name', 'First Name');
    $columns[2] = array('author_last_name', 'Last Name');
    return $columns;
  }

  function prepareRow($row) {

    if (parent::prepareRow($row) === FALSE) {
      return FALSE;
    }

    $nid = $row->nid;
    $first_name = $row->author_first_name;
    $last_name = $row->author_last_name;

    $node = node_load($nid);

    if (!$node) {
      print 'no node for nid = '.$nid.PHP_EOL;
      return FALSE;
    }

    $ewrapper = entity_metadata_wrapper('node', $node);
    if (empty($ewrapper->field_authors)) {
      return FALSE;
    }

    // get the current list of authors saved with the node
    $authors = $ewrapper->field_authors->value();

    $found = false;
    foreach($authors as $author) {
      $ewrap_author = entity_metadata_wrapper('field_collection_item', $author);
      $fn = $ewrap_author->field_first_name->value();
      $ln = $ewrap_author->field_last_name->value();
      if ($fn == $first_name && $ln == $last_name) {
        $found = true;
        break;
      }
    }

    if ($found) {
      return FALSE;
    }

    $row->faculty_nid = 0;
    $row->fac_or_other = 'Other';
    
    return TRUE;
  }
  
  /**
   * Define additional computed fields.
   * @return array
   */
  protected function fields() {
    $fields = array();
    $fields['fac_or_other'] = 'Field switcher for Authors field collection. Possible values: UseEntityReference, Other';
    $fields['faculty_nid'] = 'Faculty NID looked up by sunetid in prepareRow().';
    return $fields;
  }
}
