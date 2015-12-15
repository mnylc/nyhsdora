<?php
/**
 * @file
 * Callbacks used in derivatives hook.
 */

/**
 * Add thumbnail, based on first member.
 */
function nyhsdora_compound_derive_thumbnail_of_parent(AbstractObject $compound, $force = TRUE) {
  if ($force || !isset($compound['TN'])) {
    islandora_compound_object_update_parent_thumbnail($compound);
    if (isset($compound['TN'])) {
      return array(
        'success' => TRUE,
        'messages' => array(
          array(
            'message' => t('Updated @dsid on @parent.'),
            'message_sub' => array(
              '@dsid' => 'TN',
              '@parent' => $compound->id,
            ),
            'type' => 'dsm',
            'severity' => 'status',
          ),
        ),
      );
    }
    else {
      return array(
        'success' => FALSE,
        'messages' => array(
          array(
            'message' => t('Failed to update @dsid on @parent.'),
            'message_sub' => array(
              '@dsid' => 'TN',
              '@parent' => $compound->id,
            ),
            'type' => 'watchdog',
            'severity' => WATCHDOG_ERROR,
          ),
        ),
      );
    }
  }
}
/**
 * Add thumbnail to parent compound, if first member.
 */
function nyhsdora_compound_derive_thumbnail_on_parent(AbstractObject $child, $force = TRUE) {
  $compound_info = islandora_compound_object_retrieve_compound_info($child);
  if (empty($compound_info)) {
    return;
  }
  $compound = islandora_object_load($compound_info['parent_pid']);
  $is_first_child_of_compound = empty($compound_info['previous_pid']);
  $can_derive = !isset($compound['TN']) || $force;
  if ($is_first_child_of_compound && $can_derive && is_object($compound)) {
    islandora_compound_object_update_parent_thumbnail($compound);
    if (isset($child['TN'])) {
      return array(
        'success' => TRUE,
        'messages' => array(
          array(
            'message' => t('Updated @dsid on @parent.'),
            'message_sub' => array(
              '@dsid' => 'TN',
              '@parent' => $compound->id,
            ),
            'type' => 'dsm',
            'severity' => 'status',
          ),
        ),
      );
    }
    else {
      return array(
        'success' => FALSE,
        'messages' => array(
          array(
            'message' => t('Failed to update @dsid on @parent.'),
            'message_sub' => array(
              '@dsid' => 'TN',
              '@parent' => $compound->id,
            ),
            'type' => 'watchdog',
            'severity' => WATCHDOG_ERROR,
          ),
        ),
      );
    }
  }
}
