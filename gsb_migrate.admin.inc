<?php

/**
 * GSB Migrate config form.
 */
function gsb_migrate_config_form() {
  // Reuse Migrate D2D wizard form.
  $migrate_d2d_wizard = new MigrateD2DWizard();
  $form = $migrate_d2d_wizard->sourceDataForm($form_state);
  $form['description']['#markup'] = t("We don't expose connection info, please re-enter connection info if needed.");
  unset($form['overview']);

  // Check if database connection already set.
  $database = variable_get('gsb_migrate_database_connection', array());
  if (!empty($database)) {
    $form['#prefix'] = "<h2>Connection already set</h2>";
  }

  $form['actions']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save'),
  );

  return $form;
}

/**
 * Validate GSB migrate config form.
 * @param $form
 * @param $form_state
 */
function gsb_migrate_config_form_validate($form, &$form_state) {
  $database = $form_state['values'][$form_state['values']['driver']];
  $database['driver'] = $form_state['values']['driver'];
  $form_state['storage']['database'] = $database;

  // Make sure we can connect
  try {
    Database::addConnectionInfo('gsb_legacy', 'default', $database);
    $connection = Database::getConnection('default', 'gsb_legacy');

    $admin_uid = $connection->select('users', 'u')
      ->fields('u', array('uid'))
      ->condition('uid', 1)
      ->execute()
      ->fetchField();
    if (!$admin_uid) {
      form_set_error('driver', t('The source database does not appear to be a Drupal database.'));
    }
  }
  catch (Exception $e) {
    form_set_error('driver', t('Failed to connect to source database, message returned: %message',
      array('%message' => $e->getMessage())));
    return;
  }
}

/**
 * Submit GSB migrate config form.
 * @param $form
 * @param $form_state
 */
function gsb_migrate_config_form_submit($form, &$form_state) {
  // Save database connection;
  variable_set('gsb_migrate_database_connection', $form_state['storage']['database']);
}
