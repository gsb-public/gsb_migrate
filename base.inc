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
