<?php

/**
 * Our DrupalVersion implementation with some useful functionality.
 *
 *  - Pull source field_collection entities' fields.
 */
class GSBDrupal extends DrupalVersion7 {

  /**
   * Override populateSourceFieldInfo,
   */
  protected function populateSourceFieldInfo($entity_type, $bundle, $include_body = FALSE) {
    parent::populateSourceFieldInfo($entity_type, $bundle, $include_body);

    // Populate SourceFieldInfo with field collection fields.
    $this->populateFieldCollectionSourceFieldInfo();
  }

  /**
   * Append field_collection fields to souceFieldInfo.
   */
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

  /**
   * Override getSourceValues.
   *  - Get values for source field_collection fields.
   */
  public function getSourceValues($row, $entity_id) {
    parent::getSourceValues($row, $entity_id);
    // Load up field data for dynamically mapped fields
    foreach ($this->sourceFieldInfo as $field_name => $info) {
      if ($info['type'] == 'field_collection' && !empty($row->{$field_name})) {
        if (isset($this->sourceFieldInfo[$field_name]['field_collection_fields'])) {
          foreach ($this->sourceFieldInfo[$field_name]['field_collection_fields'] as $fc_field_name => $columns) {
            foreach ($row->{$field_name} as $delta => $fc_entity_id) {
              // Find the data in field_data_$fc_field_name.
              $table = "field_data_$fc_field_name";
              $result = Database::getConnection('default', $this->arguments['source_connection'])
                ->select($table, 'f')
                ->fields('f')
                ->condition('entity_type', 'field_collection_item')
                ->condition('bundle', $field_name)
                ->condition('entity_id', $fc_entity_id)
                ->orderBy('delta')
                ->execute();
              foreach ($result as $field_row) {
                foreach ($columns as $display_name => $column_name) {
                  if (isset($row->$display_name) && !is_array($row->$display_name)) {
                    $row->$display_name = array($row->$display_name);
                  }
                  $row->{$display_name}[$delta][] = $field_row->$column_name;
                }
              }
            }
          }
        }
      }
    }
  }

  /**
   * Override getSourceFields.
   *  - Add field collection source fields.
   */
  public function getSourceFields($entity_type, $bundle, $include_body = FALSE) {
    $fields = parent::getSourceFields($entity_type, $bundle, $include_body);
    foreach ($this->sourceFieldInfo as $field_name => $info) {
      if ($info['type'] == 'field_collection') {
        if (isset($info['field_collection_fields'])) {
          foreach ($info['field_collection_fields'] as $field_name => $columns) {
            foreach ($columns as $display_name => $column_name) {
              $fields[$display_name] = t('!label subfield', array('!label' => $info['label']));
            }
          }
        }
      }
    }

    return $fields;
  }
}

/**
 * Abstract base class for Node migrations from CSV source.
 */
abstract class GSBCSVNodeMigration extends Migration {
  public function __construct(array $arguments) {
    parent::__construct($arguments);

    // Create a MigrateSource object, which manages retrieving the input data.
    $this->source = new MigrateSourceCSV($arguments['source_file'], $this->csvcolumns(), array('embedded_newlines' => TRUE));
    $this->map = new MigrateSQLMap($this->machineName,
      array(
        'nid' => array(
          'type' => 'int',
          'not null' => true,
        ),
      ),
      MigrateDestinationNode::getKeySchema()
    );

    $this->destination = new MigrateDestinationNode($arguments['destination_type']);

    $this->addSimpleMappings(array('title', 'status'));
    $this->addFieldMapping('uid')
      ->defaultValue(1);
    $this->addFieldMapping('field_editorial_summary', 'teaser');
  }

  /**
   * Override this method to specify csv columns mapping.
   */
  abstract public function csvcolumns();

  public function prepareRow($row) {
    if (parent::prepareRow($row) === FALSE) {
      return FALSE;
    }

    // Left here for debugging directly in console.
    print $row->csvrownum . '. ' . $row->title . "\n";
  }

  /**
   * Implement createStub method to resolve chicken-egg migration problems.
   */
  protected function createStub(Migration $migration, array $source_id) {
    $node = new stdClass();
    $node->title = t('Stub for @id', array('@id' => $source_id[0]));
    $node->type = $migration->destination->getBundle();
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
 * Class GSBMigrateHelper.
 *  - Encapsulates useful callbacks to transform and prepare data for the import.
 */
class GSBMigrateHelper {

  /**
   * Helper callback to convert year into Drupal-friedly date.
   * @todo: Probably move to GSBMigrateDateConverter.
   *
   * @param $year
   * @return bool|string
   *   Returns date if date format is correct, FALSE otherwise.
   */
  public static function yearToDate($year) {
    if ($date = date_create_from_format('Y-m-d H:i:s', $year . '-01-01 00:00:00')) {
      return date_format($date, 'Y-m-d H:i:s');
    }
  }

  /**
   * Trims all array members.
   */
  public static function arrayTrim($array) {
    if (is_array($array)) {
      return array_map('trim', $array);
    }
    else {
      return $array;
    }
  }

  /**
   * Converts array members to lowercase.
   */
  public static function arrayStrToLower($array) {
    if (is_array($array)) {
      return array_map('strtolower', $array);
    }
    else {
      return $array;
    }
  }

  /**
   * Lookup tids by taxonomy term names for Key Taxonomy field.
   *
   * Key taxonomy is a entity reference that references 3 vocabularies.
   * We need to lookup term tids by term names for those vocabularies.
   */
  public static function keyTaxonomyTidsLookup($term_names) {
    $terms = array();
    foreach ($term_names as $term_name) {
      // Skip empty strings.
      if (empty($term_name)) {
        continue;
      }

      $tid = db_query(
        "SELECT tid FROM {taxonomy_term_data}
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

    return $terms;
  }

  /**
   * Convert profile visibility values from Value to Key.
   */
  public static function visibilityTextToId($value) {
    return array_search($value, array(
      0 => 'Hidden',
      1 => 'Visible',
      2 => 'Featured'
    ));
  }
}

/**
 * GSBMigrateDateConverter unility class.
 */
class GSBMigrateDateUtility {
  protected $format;

  public function __construct($format) {
    $this->format = $format;
  }

  /**
   * Convert any date string to Drupal-friendly date format.
   *
   * @param $date
   * @return string
   *   Date in Drupal-friendly format.
   */
  public function convert($date) {
    if (empty($this->format)) {
      throw new Exception('GSBMigrateDateUtility format must be set prior to calling convert.');
    }

    if ($date = date_create_from_format($this->format, $date)) {
      return date_format($date, 'Y-m-d H:i:s');
    }
  }
}

/**
 * Array implementation of MigrateList.
 *  - Allows to use arrays as migration source.
 */
class GSBListArray extends MigrateList {

  protected $array;

  public function __construct($array) {
    parent::__construct();
    $this->array = $array;
  }

  public function __toString() {
    return 'Array';
  }

  public function getIdList() {
    return array_keys($this->array);
  }

  public function computeCount() {
    return count($this->array);
  }
}

/**
 * Array implementation of MigrateItem.
 *  - Allows to use arrays as migration source.
 */
class GSBItemArray extends MigrateItem {
  protected $array;

  public function __construct($array) {
    parent::__construct();
    $this->array = $array;

  }

  public function getItem($id) {
    $this->array[$id]['id'] = $id;
    return (object) $this->array[$id];
  }
}

/**
 * Helper class to construct array sources for migration by reading CSV files and splitting values with a separator.
 *
 *  -
 */
class GSBCSVItemSplitter {

  /**
   * Source CSV filename.
   */
  protected $filename;
  /**
   * Columns to process from CSV file.
   */
  protected $columns;
  /**
   * Array of required fields.
   *  - If any of required fields is empty, the row will be skipped and never appear in source.
   */
  protected $required_fields;
  /**
   * Delimiter to be used when exploding values.
   *  (Optional) '|' will be used if not specified.
   */
  protected $delimiter;

  public function __construct($filename, $columns, $options = array()) {
    $this->filename = $filename;
    $this->columns = $columns;

    // Set '|' as delimiter unless other delimiter is specified in options.
    $this->delimiter = !empty($options['delimiter']) ? $options['delimiter'] : '|';
    $this->required_fields = !empty($options['required_fields']) ? $options['required_fields'] : array();
  }

  /**
   * Main method of this class.
   *
   * Fetches data according to provided settings.
   *   - Parses the CSV.
   *   - Collects relevant columns
   *   - Splits columns using delimiter
   *   - Filters final results array.
   */
  public function fetch() {
    // Read CSV file.
    if (($handle = fopen($this->filename, "r")) !== FALSE) {
      while (($data = fgetcsv($handle, 0, ",")) !== FALSE) {
        $rows[] = $data;
      }
      fclose($handle);
    }

    $csvrownum = 1;
    // We need keys to start from 1 when added here. Unset key 0 when complete.
    $array = array(0);
    foreach ($rows as $row) {
      // only use rows specified in $columns.
      $row = array_intersect_key($row, $this->columns);
      // Set meaningful keys for the columns mentioned in $columns.
      foreach ($this->columns as $int => $values) {
        list($key, $description) = $values;
        // Copy value to more descriptive string based key and then unset original.
        $row[$key] = isset($row[$int]) ? $row[$int] : NULL;
        unset($row[$int]);
      }

      // Now split row values and create separate rows.
      $multiple_items = array();
      foreach ($this->columns as $int => $values) {
        list($key, $description) = $values;
        foreach (explode($this->delimiter, $row[$key]) as $delta => $value) {
          $multiple_items[$csvrownum][$delta] = isset($multiple_items[$csvrownum][$delta]) ? $multiple_items[$csvrownum][$delta] : $row;
          $multiple_items[$csvrownum][$delta][$key] = trim($value);
        }
      }

      // Filter out not valid results and add valid ones to the bucket.
      foreach ($multiple_items as $items_from_a_row) {
        foreach ($items_from_a_row as $single_item) {
          if ($this->isValid($single_item)) {
            $array[] = $single_item;
          }
        }
      }

      $csvrownum++;
    }

    // Unset first fake value that was previously added.
    unset($array[0]);

    return $array;
  }

  /**
   * Validate items
   */
  protected function isValid($item) {
    // Validate items by required fields. If at least one field fails, return FALSE.
    foreach ($this->required_fields as $field) {
      if (empty($item[$field])) {
        return FALSE;
      }
    }

    return TRUE;
  }
}
