<?php


/**
 * Class GSBIdeaStoryMigration
 *  - Migrate class for Idea Story CT migration.
 */
class GSBIdeaStoryMigration extends GSBCSVNodeMigration {
  public function __construct(array $arguments) {
    parent::__construct($arguments);

    $this->addFieldMapping('field_body', 'body');
    $this->addFieldMapping('field_body:format')
      ->defaultValue('markdown');

    $date_utility = new GSBMigrateDateUtility('n/j/y g:i');
    $this->addFieldMapping('field_date', 'publication_date')
      ->callbacks(array($date_utility, 'convert'));

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
      ->separator('|')
      ->callbacks(array('GSBMigrateHelper', 'arrayTrim'));
    $this->addFieldMapping('field_link_unlimited:title', 'related_link_text')
      ->separator('|')
      ->callbacks(array('GSBMigrateHelper', 'arrayTrim'));
    $this->addFieldMapping('field_key_taxonomy', 'key_taxonomy')
      ->separator('|')
      ->callbacks(
        array('GSBMigrateHelper', 'arrayTrim'),
        array('GSBMigrateHelper', 'keyTaxonomyTidsLookup')
      );

    $this->addFieldMapping('field_related_idea_story', 'related_story')
      ->separator('|')
      ->callbacks(array('GSBMigrateHelper', 'arrayTrim'))
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
    $this->addFieldMapping('field_tag', 'keywords')
      // Tags are lowercase.
      ->callbacks('strtolower');
    $this->addFieldMapping('field_tag:create_term')
      // For tags we want to create terms.
      ->defaultValue(TRUE);

    $date_utility = new GSBMigrateDateUtility('m-d-y');
    $this->addFieldMapping('field_rethink_date', 'rethink_date')
      ->callbacks(array($date_utility, 'convert'));

    $this->addFieldMapping('field_part_of_rethink', 'part_of_rethink');

    $this->addFieldMapping('field_year', 'stanford_business_year')
      ->callbacks(array('GSBMigrateHelper', 'yearToDate'));
    $this->addFieldMapping('field_part_of_business_magazine', 'part_of_business_magazine');
    $this->addFieldMapping('field_volume_number', 'stanford_business_volume');
    $this->addFieldMapping('field_issue_number', 'stanford_business_issue');

    $this->addFieldMapping('field_season', 'stanford_business_season')
      ->callbacks('strtolower');

    $this->addFieldMapping('field_alumni_story', 'alumni_story');
    $this->addFieldMapping('field_program_single', 'alumni_program');
    $this->addFieldMapping('field_program_single:create_term')
      ->defaultValue(FALSE);

    // Create redirects.
    $this->addFieldMapping('migrate_redirects', 'path');
  }

  /**
   * Define field names for CSV columns.
   */
  public function csvcolumns() {
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
    $columns[38]  = array('keywords', 'Keywords');
    $columns[39]  = array('alumni_program', 'Alumni Program');
    $columns[40]  = array('stanford_business_volume', 'Stanford Business Volume');
    $columns[41]  = array('stanford_business_issue', '"Stanford Business Issue"');
    $columns[42]  = array('resource_twitter_name', 'Resource Twitter Name');
    $columns[43]  = array('resource_twitter_url', 'Resource Twitter URL');

    return $columns;
  }

  function prepareRow($row) {
    if (parent::prepareRow($row) === FALSE) {
      return FALSE;
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

    // Check Rethink checkbox if field_rethink_date is set.
    if (!empty($row->rethink_date)) {
      $row->part_of_rethink = TRUE;
    }

    // Check Business Magazine checkbox if field_program_single is set.
    if (!empty($row->stanford_business_year)) {
      $row->part_of_business_magazine = TRUE;
    }

    return TRUE;
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

    return TRUE;
  }
}
