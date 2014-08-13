<?php


/**
 * Class GSBIdeaStoryMigration
 *  - Migrate class for Idea Story CT migration.
 */
class GSBIdeaStoryMigration extends Migration {
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

    $this->destination = new MigrateDestinationNode('idea_story');

    $this->addSimpleMappings(array('title', 'status'));
    $this->addFieldMapping('uid')
      ->defaultValue(1);
    $this->addFieldMapping('field_editorial_summary', 'teaser');
    $this->addFieldMapping('field_body', 'body');
    $this->addFieldMapping('field_body:format')
      ->defaultValue('markdown');
    $this->addFieldMapping('field_date', 'publication_date');
    $this->addFieldMapping('field_icon')
      ->defaultValue('written');

    // Generate a short URL for field_link_single using some URL shortener API.
    //$this->addFieldMapping('field_link_single', 'link_to_article');

    $this->addFieldMapping('field_image_key_image', 'key_image');
    $this->addFieldMapping('field_image_key_image:file_replace')
      ->defaultValue(FILE_EXISTS_REPLACE);
    $this->addFieldMapping('field_image_key_image:preserve_files')
      ->defaultValue(TRUE);
    $this->addFieldMapping('field_image_key_image:alt', 'key_image_alt');
    $this->addFieldMapping('field_link_unlimited', 'related_link_url')
      ->separator(' | ');
    $this->addFieldMapping('field_link_unlimited:title', 'related_link_text')
      ->separator(' | ');
    $this->addFieldMapping('field_key_taxonomy', 'key_taxonomy');

    $this->addFieldMapping('field_person_fac_ref', 'related_faculty')
      ->separator(' | ');
    $this->addFieldMapping('field_related_idea_story', 'related_story')
      ->separator(' | ')
      ->sourceMigration('GSBIdeaStory');

    $this->addFieldMapping('field_academic_area_unlimited', 'academic_area');
    $this->addFieldMapping('field_academic_area_unlimited:create_term')
      ->defaultValue(FALSE);
    $this->addFieldMapping('field_business_insight_topic', 'additional_topics');
    $this->addFieldMapping('field_business_insight_topic:create_term')
      ->defaultValue(FALSE);
    $this->addFieldMapping('field_region_of_interest', 'region');
    $this->addFieldMapping('field_region_of_interest:create_term')
      ->defaultValue(FALSE);
    $this->addFieldMapping('field_centers_research_programs', 'circle');
    $this->addFieldMapping('field_centers_research_programs:create_term')
      ->defaultValue(FALSE);
    $this->addFieldMapping('field_tag', 'tag');
    $this->addFieldMapping('field_tag:create_term')
      ->defaultValue(FALSE);

    $this->addFieldMapping('field_alumni_story', 'alumni_story');
    $this->addFieldMapping('field_program_single', 'alumni_program');
    $this->addFieldMapping('field_program_single:create_term')
      ->defaultValue(FALSE);
  }

  /**
   * Define field names for CSV columns.
   */
  protected function csvcolumns() {
    $columns[0]   = array('status', 'Status');
    $columns[1]   = array('nid', 'NID');
    $columns[2]   = array('publication_date', 'Publication date');
    $columns[3]   = array('title', 'Title');
    $columns[4]   = array('nothing', 'Not used');
    $columns[5]   = array('path', 'Path');
    $columns[6]   = array('teaser', 'Teaser');
    $columns[7]   = array('nothing_1', 'Not used');
    $columns[8]   = array('body', 'Body');
    $columns[9]   = array('byline_first_name', 'Byline First Name');
    $columns[10]  = array('byline_last_name', 'Byline Last Name');
    $columns[11]  = array('related_faculty', 'Related Faculty');
    $columns[12]  = array('related_content_label', 'Related Content Label');
    $columns[13]  = array('related_content', 'Related Content');
    $columns[14]  = array('related_story', 'Related Story');
    $columns[15]  = array('related_link_text', 'Related Link Text');
    $columns[16]  = array('related_link_url', 'Related Link URL');
    $columns[17]  = array('rethink_date', 'Re:Think Date');
    $columns[18]  = array('stanford_business_season', 'Stanford Business Season');
    $columns[19]  = array('stanford_business_year', 'Stanford Business Year');
    $columns[20]  = array('resource_facebook_post_name', 'Resource Facebook Post Name');
    $columns[21]  = array('resource_facebook_post_url', 'Resource Facebook Post URL');
    $columns[22]  = array('resource_video_name', 'Resource Video Name');
    $columns[23]  = array('resource_video_url', 'Resource Video URL');
    $columns[24]  = array('resource_image_name', 'Resource Image Name');
    $columns[25]  = array('resource_image_file', 'Resource Image File');
    $columns[26]  = array('resource_image_caption', 'Resource Image Caption');
    $columns[27]  = array('resource_image_alt', 'Resource Image Alt Text');
    $columns[28]  = array('resource_callout_name', 'Resource Callout Name');
    $columns[29]  = array('resource_callout_text', 'Resource Callout Text');
    $columns[30]  = array('resource_callout_attribution', 'Resource Callout Attribution');
    $columns[31]  = array('key_image', 'Key Image');
    $columns[32]  = array('key_image_alt', 'Key Image Alt Text');
    $columns[33]  = array('key_taxonomy', 'Key Taxonomy');
    $columns[34]  = array('academic_area', 'Academic Area');
    $columns[35]  = array('additional_topics', 'Additional Topics');
    $columns[36]  = array('region', 'Region of Interest');
    $columns[37]  = array('circle', 'Circle');
    $columns[38]  = array('tag', 'Tag');
    $columns[39]  = array('alumni_program', 'Alumni Program');

    return $columns;
  }

  function prepareRow($row) {
    if (parent::prepareRow($row) === FALSE) {
      return FALSE;
    }

    // Left here for debugging.
    print $row->csvrownum . '. ' . $row->title . "\n";

    if (!empty($row->publication_date)) {
      if ($date = date_create_from_format('m/d/Y G:i:s', $row->publication_date)) {
        $row->publication_date = date_format($date, 'Y-m-d H:i:s');
      }
    }

    // Key taxonomy is a entity reference that references 3 vocabularies.
    // We need to lookup term tids by term names for those vocabularies.
    if (!empty($row->key_taxonomy)) {
      $terms = array();
      $term_names = explode('|', $row->key_taxonomy);
      foreach ($term_names as $term_name) {
        $term_name = trim($term_name);
        $tid = db_query("SELECT tid FROM {taxonomy_term_data}
                         WHERE name = :name
                         AND vid IN (:vid)",
          array(':name' => $term_name, ':vid' => array('101', '106', '196')))->fetchField();
        if ($tid) {
          // Entity reference field wants an entity id.
          $terms[] = $tid;
        }
        else {
          print 'Key Taxonomy term ' . $term_name . ' doesn\'t exist.' . "\n";
        }
      }

      if (!empty($terms)) {
        $row->key_taxonomy = $terms;
      }
    }

    // Check Alumni Story checkbox if field_program_single is set.
    if (!empty($row->alumni_program)) {
      $row->alumni_story = TRUE;
    }

    if (!empty($row->key_image) && strpos($row->key_image, 'imgur.com') !== FALSE) {
      // URLs in CSV are of http://imgur.com/JYDJnfY format. We'll just dirty-convert them to publicly downloadable http://i.imgur.com/JYDJnfY.jpg
      $row->key_image = str_replace('imgur.com', 'i.imgur.com', $row->key_image) . '.jpg';
    }

    // Avoid empty string lookup when creating stubs.
    if (empty($row->related_story)) {
      unset($row->related_story);
    }

    return TRUE;
  }


  protected function createStub(Migration $migration, array $source_id) {
    $node = new stdClass();
    $node->title = t('Stub for @id', array('@id' => $source_id[0]));
    $node->type = $this->destination->getBundle();
    $node->uid = 1;
    $node->status = 0;
    node_save($node);
    if (isset($node->nid)) {
      return array($node->nid);
    }
    else {
      return FALSE;
    }
  }
}

/**
 * Class GSBIdeaStoryBylineMigration
 *  - Migrate class for Idea Story Byline migration.
 */
class GSBIdeaStoryBylineMigration extends Migration {
  public function __construct(array $arguments) {
    parent::__construct($arguments);
    $this->dependencies = array('GSBIdeaStory');

    // Create a MigrateSource object, which manages retrieving the input data.
    $this->source = new MigrateSourceCSV($arguments['source_file'], $this->csvcolumns(), array(), $this->fields());
    $this->map = new MigrateSQLMap($this->machineName,
      array(
        'nid' => array(
          'type' => 'int',
          'not null' => TRUE,
        ),
        'byline_first_name' => array(
          'type' => 'varchar',
          'length' => 255,
          'default' => '',
        ),
        'byline_last_name' => array(
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
      ->sourceMigration('GSBIdeaStory');

    $this->addFieldMapping('field_person_fac_or_other', 'fac_or_other');
    $this->addFieldMapping('field_person_fac_single_ref', 'faculty_nid');
    $this->addFieldMapping('field_first_name', 'byline_first_name');
    $this->addFieldMapping('field_last_name', 'byline_last_name');
  }

  /**
   * Define additional computed fields.
   * @return array
   */
  protected function fields() {
    $fields = array();

    $fields['fac_or_other'] = 'Field switcher for Authors field collection. Possible values: UseEntityReference, Other';

    return $fields;
  }

  /**
   * Define field names for CSV columns.
   */
  protected function csvcolumns() {
    $columns[0] = array('nid', 'Nid');
    $columns[1] = array('byline_first_name', 'Byline First Name');
    $columns[2] = array('byline_last_name', 'Byline Last Name');
    $columns[3] = array('faculty_nid', 'Faculty NID');

    return $columns;
  }

  function prepareRow($row) {
    if (parent::prepareRow($row) === FALSE) {
      return FALSE;
    }

    // Left here for debugging.
    print $row->nid . "\n";

    if (!empty($row->faculty_nid)) {
      $row->fac_or_other = 'UseEntityReference';
    }

    // Set fac_or_other to Other in case we have simple First Name and Last Name.
    if (!empty($row->byline_first_name) && !empty($row->byline_last_name)) {
      $row->fac_or_other = 'Other';
    }

//    // Ensure we have a value for faculty_nid.
//    if (empty($row->faculty_nid)) {
//      $row->faculty_nid = 0;
//    }

    return TRUE;
  }
}

/**
 * Class GSBIdeaStoryRelatedContentMigration
 *  - Migrate class for Idea Story Related Content migration.
 */
class GSBIdeaStoryRelatedContentMigration extends Migration {
  public function __construct(array $arguments) {
    parent::__construct($arguments);
    $this->dependencies = array('GSBIdeaStory');

    // Create a MigrateSource object, which manages retrieving the input data.
    $this->source = new MigrateSourceCSV($arguments['source_file'], $this->csvcolumns());
    $this->map = new MigrateSQLMap($this->machineName,
      array(
        'nid' => array(
          'type' => 'int',
          'not null' => TRUE,
        ),
        'target_id' => array(
          'type' => 'int',
          'not null' => TRUE,
        ),
      ),
      MigrateDestinationFieldCollection::getKeySchema()
    );

    $this->destination = new MigrateDestinationFieldCollection(
      'field_related_other_unlimited',
      array('host_entity_type' => 'node')
    );

    $this->addFieldMapping('host_entity_id', 'nid')
      ->sourceMigration('GSBIdeaStory');

    $this->addFieldMapping('field_related_other_label', 'label');
    $this->addFieldMapping('field_related_other_content', 'target_id');
  }

  /**
   * Define field names for CSV columns.
   */
  protected function csvcolumns() {
    $columns[0] = array('nid', 'Nid');
    $columns[1] = array('label', 'Related Content Label');
    $columns[2] = array('target_id', 'Related Content Nid');

    return $columns;
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
 * Class GSBIdeaStoryResourceImageMigration
 *  - Migrate class for Idea Story Resource Image migration.
 */
class GSBIdeaStoryResourceImageMigration extends Migration {
  public function __construct(array $arguments) {
    parent::__construct($arguments);
    $this->dependencies = array('GSBIdeaStory');

    // Create a MigrateSource object, which manages retrieving the input data.
    $this->source = new MigrateSourceCSV($arguments['source_file'], $this->csvcolumns());
    $this->map = new MigrateSQLMap($this->machineName,
      array(
        'csvrownum' => array(
          'type' => 'int',
          'not null' => TRUE,
        ),
      ),
      MigrateDestinationFile::getKeySchema()
    );

    $this->destination = new MigrateDestinationFile('resource_image', 'MigrateFileUri');
    $this->addFieldMapping('value', 'resource_image_uri');
    // @todo: Add filename and type to available destination fields. Or consider resource_type field.
    $this->addFieldMapping('filename', 'resource_image_name');
    $this->addFieldMapping('type')->defaultValue('resource_image');

    $this->addFieldMapping('resource_host_node', 'nid')
      ->sourceMigration('GSBIdeaStory');
  }

  /**
   * Define field names for CSV columns.
   */
  protected function csvcolumns() {
    $columns[1] = array('nid', 'Idea Story NID');
    $columns[24] = array('resource_image_name', 'Resource Name');
    $columns[25] = array('resource_image_uri', 'Resource Image URI');
    $columns[26] = array('resource_image_caption', 'Resource Image Caption');
    $columns[27] = array('resource_image_alt', 'Resource Image Alt');

    return $columns;
  }

  function prepareRow($row) {
    if (parent::prepareRow($row) === FALSE) {
      return FALSE;
    }

    if (empty($row->resource_image_uri)) {
      return FALSE;
    }
    // Left here for debugging.
    print $row->csvrownum . "\n";

    return TRUE;
  }
}

/**
 * Class GSBIdeaStoryResourceYoutubeMigration
 *  - Migrate class for Idea Story Resource Youtube migration.
 */
class GSBIdeaStoryResourceYoutubeMigration extends Migration {
  public function __construct(array $arguments) {
    parent::__construct($arguments);
    $this->dependencies = array('GSBIdeaStory');

    // Create a MigrateSource object, which manages retrieving the input data.
    $this->source = new MigrateSourceCSV($arguments['source_file'], $this->csvcolumns());
    $this->map = new MigrateSQLMap($this->machineName,
      array(
        'csvrownum' => array(
          'type' => 'int',
          'not null' => TRUE,
        ),
      ),
      MigrateDestinationFile::getKeySchema()
    );

    $this->destination = new MigrateDestinationFile('resource_youtube', 'ResourcesResourcesFile');
    //$this->addFieldMapping('value', 'resource_video_url');
    $this->addFieldMapping('field_youtube_embed_code', 'resource_video_url');
    $this->addFieldMapping('filename', 'resource_video_name');
    $this->addFieldMapping('resource_type')->defaultValue('resource_youtube');

    $this->addFieldMapping('resource_host_node', 'nid')
      ->sourceMigration('GSBIdeaStory');
  }

  /**
   * Define field names for CSV columns.
   */
  protected function csvcolumns() {
    $columns[1] = array('nid', 'Idea Story NID');
    $columns[22]  = array('resource_video_name', 'Resource Video Name');
    $columns[23]  = array('resource_video_url', 'Resource Video URL');

    return $columns;
  }

  function prepareRow($row) {
    if (parent::prepareRow($row) === FALSE) {
      return FALSE;
    }

    if (empty($row->resource_video_url)) {
      return FALSE;
    }
    // Left here for debugging.
    print $row->csvrownum . "\n";

    return TRUE;
  }
}

/**
 * Class GSBIdeaStoryResourceYoutubeMigration
 *  - Migrate class for Idea Story Resource Youtube migration.
 */
class GSBIdeaStoryResourceCalloutMigration extends Migration {
  public function __construct(array $arguments) {
    parent::__construct($arguments);
    $this->dependencies = array('GSBIdeaStory');

    // Create a MigrateSource object, which manages retrieving the input data.
    $this->source = new MigrateSourceCSV($arguments['source_file'], $this->csvcolumns());
    $this->map = new MigrateSQLMap($this->machineName,
      array(
        'csvrownum' => array(
          'type' => 'int',
          'not null' => TRUE,
        ),
      ),
      MigrateDestinationFile::getKeySchema()
    );

    $this->destination = new MigrateDestinationFile('resource_pull_quote', 'ResourcesResourcesFile');
    $this->addFieldMapping('filename', 'resource_callout_name');
    $this->addFieldMapping('field_description', 'resource_callout_text');
    $this->addFieldMapping('field_quote_author', 'resource_callout_attribution');
    $this->addFieldMapping('resource_type')->defaultValue('resource_callout');

    $this->addFieldMapping('resource_host_node', 'nid')
      ->sourceMigration('GSBIdeaStory');
  }

  /**
   * Define field names for CSV columns.
   */
  protected function csvcolumns() {
    $columns[1] = array('nid', 'Idea Story NID');
    $columns[28]  = array('resource_callout_name', 'Resource Callout Name');
    $columns[29]  = array('resource_callout_text', 'Resource Callout Text');
    $columns[30]  = array('resource_callout_attribution', 'Resource Callout Attribution');

    return $columns;
  }

  function prepareRow($row) {
    if (parent::prepareRow($row) === FALSE) {
      return FALSE;
    }

    if (empty($row->resource_callout_name)) {
      return FALSE;
    }
    // Left here for debugging.
    print $row->csvrownum . "\n";

    return TRUE;
  }
}
