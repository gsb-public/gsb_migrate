<?php

/**
 * Implements hook_migrate_api().
 *
 * Register migrations.
 */
function gsb_migrate_migrate_api() {
  $source_connection = 'gsb_legacy';

  $api = array(
    'api' => 2,
    'migrations' => array(
      'GSBFaculty' => array(
        'class_name' => 'GSBFacultyMigration',
        'group_name' => 'content',
        'source_connection' => $source_connection,
      ),
    ),
  );

  return $api;
}
