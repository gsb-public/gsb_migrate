<?php


/**
 * Class GSBRankingMigration
 *  - Migrate class for Ranking CT migration.
 */
class GSBRankingMigration extends GSBCSVNodeMigration {
  public function __construct(array $arguments) {
    parent::__construct($arguments);

    $date_utility = new GSBMigrateDateUtility('n/j/Y g:i:s');
    $this->addFieldMapping('field_date', 'source_date')
      ->callbacks(array($date_utility, 'convert'));

    // @todo: Is it written? CSV doesn't say anything.
    $this->addFieldMapping('field_icon')
      ->defaultValue('written');

    $this->addFieldMapping('field_publisher', 'source_publication');
    $this->addFieldMapping('field_link_single', 'source_link');
  }

  /**
   * Define field names for CSV columns.
   */
  public function csvcolumns() {
    $columns[0]   = array('status', 'Status');
    $columns[1]   = array('nid', 'NID');
    $columns[2]   = array('source_date', 'Source date');
    $columns[3]   = array('title', 'Title');
    $columns[4]   = array('nothing', 'Not used');
    $columns[5]   = array('source_publication', 'Source Publication');
    $columns[6]   = array('source_link', 'Source Link');
    $columns[7]   = array('teaser', 'Dek');

    return $columns;
  }

  function prepareRow($row) {
    if (parent::prepareRow($row) === FALSE) {
      return FALSE;
    }

    // Check Alumni Story checkbox if field_program_single is set.
    $row->alumni_story = !empty($row->alumni_program) ? TRUE : FALSE;
    // Check Business Magazine checkbox if field_program_single is set.
    $row->part_of_business_magazine = !empty($row->stanford_business_year) ? TRUE : FALSE;
    // Check Rethink checkbox if field_rethink_date is set.
    $row->part_of_rethink = !empty($row->rethink_date) ? TRUE : FALSE;

    // @todo: Extract to base class.
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
}
