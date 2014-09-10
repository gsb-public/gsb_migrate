<?php


/**
 * Class GSBOffsiteIdeaStoryMigration
 *  - Migrate class for Off-site Idea Story CT migration.
 */
class GSBOffsiteIdeaStoryMigration extends GSBCSVNodeMigration {
  public function __construct(array $arguments) {
    parent::__construct($arguments);

    $date_utility = new GSBMigrateDateUtility('n/j/y g:i');
    $this->addFieldMapping('field_date', 'publication_date')
      ->callbacks(
        'trim',
        array($date_utility, 'convert')
      );

    $this->addFieldMapping('field_icon', 'source_format')
      ->callbacks('strtolower');

    $this->addFieldMapping('field_image_key_image', 'key_image');
    $this->addFieldMapping('field_image_key_image:file_replace')
      ->defaultValue(FILE_EXISTS_REPLACE);
    $this->addFieldMapping('field_image_key_image:preserve_files')
      ->defaultValue(TRUE);
    $this->addFieldMapping('field_image_key_image:alt', 'key_image_alt');
    $this->addFieldMapping('field_key_taxonomy', 'key_taxonomy')
      ->separator('|')
      ->callbacks(
        array('GSBMigrateHelper', 'arrayTrim'),
        array('GSBMigrateHelper', 'keyTaxonomyTidsLookup')
      );

    $this->addFieldMapping('field_centers_research_programs', 'circle');
    $this->addFieldMapping('field_centers_research_programs:create_term')
      ->defaultValue(FALSE);
    $this->addFieldMapping('field_tag', 'keywords')
      // Tags are lowercase.
      ->callbacks('strtolower');
    $this->addFieldMapping('field_tag:create_term')
      // For tags we want to create terms.
      ->defaultValue(TRUE);

    $this->addFieldMapping('field_link_single', 'source_link');
    $this->addFieldMapping('field_publisher', 'source_publication');
    $this->addFieldMapping('field_alumni_story', 'alumni_story');

    $this->addFieldMapping('field_program_single', 'alumni_program')
      // Use seoarator even if this field is singlevalued to correct errors when more than one value is supplied.
      // It will take the first value in that case.
      ->separator('|')
      ->callbacks(array('GSBMigrateHelper', 'arrayTrim'));
    $this->addFieldMapping('field_program_single:create_term')
      ->defaultValue(FALSE);
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
    $columns[5]   = array('source_publication', 'Source publication');
    $columns[6]   = array('source_link', 'Source link');
    $columns[7]   = array('teaser', 'Teaser');
    $columns[8]   = array('nothing_1', 'Not used');
    $columns[9]   = array('source_format', 'Source Format');
    $columns[10]  = array('key_image', 'Key Image');
    $columns[11]  = array('key_image_alt', 'Key Image Alt Text');
    $columns[12]  = array('related_faculty', 'Related Faculty');
    $columns[13]  = array('display_on_profile', 'Display on Profile');
    $columns[14]  = array('key_taxonomy_voc', 'Key Taxonomy Vocabulary');
    $columns[15]  = array('key_taxonomy', 'Key Taxonomy');
    $columns[16]  = array('circle', 'Circle');
    $columns[17]  = array('keywords', 'Keywords');
    $columns[18]  = array('alumni_program', 'Alumni Program');

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

    // @todo: Extract to base class.
    if (!empty($row->key_image) && strpos($row->key_image, 'imgur.com') !== FALSE) {
      // URLs in CSV are of http://imgur.com/JYDJnfY format. We'll just dirty-convert them to publicly downloadable http://i.imgur.com/JYDJnfY.jpg
      $row->key_image = str_replace('imgur.com', 'i.imgur.com', $row->key_image) . '.jpg';
    }

    return TRUE;
  }
}
