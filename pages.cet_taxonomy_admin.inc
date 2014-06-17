<?php
/**
 * @file pages.cet_taxomomy_admin.inc
 * DESCRIPTION LINE 1
 * DESCRIPTION LINE 2
 * DESCRIPTION LINE 3
 * @name pages.cet_taxomomy_admin.inc
 * @created July 1, 2013
 * @author Cognizant
 * @version $Id:$
 * @copyright
 * Proprietary and Confidential
 * Copyright (C) 2013 Cambridge University Press. All Rights Reserved.
 */



function taxonomy_management_index() {
  //setting page title
  drupal_set_title('Taxonomy Management');
  //different menus
  $element['#value'] = '<br/><div><strong>' . l('Manage Tags', 'admin/taxonomy/tag', array('attributes' => array('title' => 'Manage Tags'))) . '</strong><br/><small>Settings page for CET Tags management.</small></div>';
  $element['#value'] .= '<br/><div><strong>' . l('Manage Categories', 'admin/taxonomy/category', array('attributes' => array('title' => 'Manage Categories'))) . '</strong><br/><small>Settings page for CET Categories management.</small></div>';

  return theme('item', $element);
}


/**
 * Function to list all the tags in a table
 * @return string HTML
 */
function cet_taxonomy_admin_view_tags() {
  //Add Javascript Support
  drupal_add_js(drupal_get_path('module', 'cet_taxonomy_admin') . '/js/cet_taxonomy_admin.js');

  $vocab_tags_id = TAGS_VOCABULARY_ID;
  $records_per_page = CET_DEFAULT_PER_PAGE_NUM;
  $form = array();
  $header = array();
  $options = array();
  $default_value = array();

  $header = array(
    'name' => array('data' => t('Name'), 'field' => 'term_data.name', 'sort' => 'asc'),
    'synonyms' => array('data' => t('List of synonyms')),
    'count' => array('data' => t('Number of content associated'), 'field' => 'node_term_node_nid'),
    'actions' => array('data' => t('Actions')),
  );
  $vocab_placeholders = db_placeholders(array($vocab_tags_id), 'int');
  $query_arg = array($vocab_tags_id);
  $query = "SELECT term_data.tid AS tid, term_data.name AS term_data_name, term_data.vid AS term_data_vid,
  COUNT(node_term_node.nid) AS node_term_node_nid
  FROM {term_data} AS term_data
  LEFT JOIN {term_node} AS term_node ON term_data.tid = term_node.tid
  LEFT JOIN {node} AS node_term_node ON term_node.vid = node_term_node.vid
  WHERE term_data.vid IN ($vocab_placeholders)
  GROUP BY term_data_name, tid, term_data_vid";

  $query_count = "SELECT COUNT(DISTINCT(term_data.tid)) AS tid, term_data.name
  FROM {term_data} AS term_data
  LEFT JOIN {term_node} AS term_node ON term_data.tid = term_node.tid
  LEFT JOIN {node} AS node_term_node ON term_node.vid = node_term_node.vid
  WHERE term_data.vid IN ($vocab_placeholders)";

  //Tag Count
  $tag_count_sql = "SELECT COUNT( term_data.tid ) AS tag_count FROM term_data AS term_data WHERE term_data.vid = %d ";
  $tag_count = db_result(db_query($tag_count_sql, $vocab_tags_id));

  $form['tags_count'] = array(
    '#type' => 'item',
    '#value' => '<b>Number of tags:</b> ' . $tag_count,
  );


  $tablesort = tablesort_sql($header);
  $order_by_query = $query . $tablesort;
  $tags_result_set = pager_query($order_by_query, $records_per_page, 0, $query_count, $query_arg);


  while($tag_data = db_fetch_object($tags_result_set)) {
    $synonym_array = taxonomy_get_synonyms($tag_data->tid);
    if(!empty($synonym_array)) {
      $synonyms = implode(", ", $synonym_array);
    } else {
      $synonyms = t('N/A');
    }

    //link for view content associated with a tag
    if($tag_data->node_term_node_nid > 0) {
      $contents_link = l(t($tag_data->node_term_node_nid), 'taxonomy/term/' . $tag_data->tid);
    } else {
      $contents_link = $tag_data->node_term_node_nid;
    }

    $options[$tag_data->tid] = array(
      'name' => check_plain($tag_data->term_data_name),
      'synonyms' => $synonyms,
      'count' => $contents_link,
      'actions' => l(t('Edit'), 'admin/taxonomy/tag/edit/' . $tag_data->tid),
      '#attributes' => array ('tag_name' => $tag_data->term_data_name, 'tag_tid' => $tag_data->tid),
    );

  }

  $is_checkbox = TRUE;
  $form['tag_listings'] = array (
    '#type' => 'tableselect',
    '#title' => 'Tag Listing',
    '#header' => $header,
    '#options' => $options,
    '#multiple' => $is_checkbox,
    '#default_value' => $default_value,
    '#empty' => t('No tags are created'),
    '#attributes' => array(
      'class' => 'tag-listing-table',
    ),
  );

  $form['pager'] = array('#value' => theme('pager', NULL, $records_per_page));

  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Delete Tags'),
    '#attributes' => array('onclick' => 'return confirm_term_delete(\'.tag-listing-table tr\');'),
  );
  return $form;
}

/**
 * [cet_taxonomy_admin_view_tags_validate description]
 * @param  [type] $form       [description]
 * @param  [type] $form_state [description]
 * @return [type]             [description]
 */
function cet_taxonomy_admin_view_tags_validate($form, &$form_state) {

  $tags_selected = array_filter($form_state['values']['tag_listings']);
  if(!empty($tags_selected)) {
    //Check for the tags if they have any contents associated
    foreach($tags_selected as $tid) {
	  $count_tags[] = $tid;
      if(taxonomy_term_count_nodes($tid) > 0) {
        $associated_terms[] = $tid;
      }
    }
    if(!empty($associated_terms) && count($associated_terms) >= 1 && count($count_tags) > 1 ) {
      //Format the message in a comma seperated tag name list
      $tag_placeholders = db_placeholders($associated_terms, 'int');
      $tag_str = format_plural(count($associated_terms), 'Tag', 'Tags');
      $tag_is = format_plural(count($associated_terms), 'is', 'are');
	  $tag_it = format_plural(count($associated_terms), 'it', 'they');
      $tag_names = db_result(db_query("SELECT GROUP_CONCAT(name) FROM {term_data} WHERE tid IN ($tag_placeholders)", $associated_terms));
      form_set_error('', t('!ts !tag_names cannot be deleted because !tag_it !ti already associated with content.', array(
		    '!ts' => $tag_str,
			'!tag_it' => $tag_it,
        '!tag_names' => $tag_names,
		    '!ti' => $tag_is,
      )));
    } elseif(!empty($associated_terms) && count($associated_terms) == 1 && count($count_tags) == 1 ) {
      //Format the message in a single way
	  $tag_placeholders = db_placeholders($associated_terms, 'int');
	  $tag_names = db_result(db_query("SELECT name FROM {term_data} WHERE tid = ($tag_placeholders)", $associated_terms));
      form_set_error('', t('Tag !tag_names cannot be deleted because it is already associated with content.', array(
			'!tag_names' => $tag_names,
	  )));
    }
  } else {
    form_set_error('', t('No tags selected.'));
  }
}

/**
 * [cet_taxonomy_admin_view_tags_submit description]
 * @param  [type] $form       [description]
 * @param  [type] $form_state [description]
 * @return [type]             [description]
 */
function cet_taxonomy_admin_view_tags_submit($form, &$form_state) {
  global $user;
  $tags_selected = array_filter($form_state['values']['tag_listings']);

  if(!empty($tags_selected)) {
    //Delete the tags
    foreach($tags_selected as $tid) {
      //for audit trail ...
      $old_values = array();
      $tag_data = taxonomy_get_term($tid);
      $audit_type = 'DEL';
      $record_name = 'Tag';
      $old_values['tag_name'] = $tag_data->name;
      if(function_exists('taxonomy_audit_trail_record_process')){
        taxonomy_audit_trail_record_process($record_name, $audit_type, $old_values);
      }
      //audit trail ends...
      taxonomy_del_term($tid);
    }

    drupal_set_message(format_plural(count($tags_selected), 'Tag deleted successfully', 'Tags deleted successfully'), 'status');
  }
}

/**
 * [admin_add_tag description]
 * @return [type] [description]
 */
function admin_edit_tag() {
  $form = array();

  $op = arg(3);
  $term_data = array();
  if($op == 'edit') {
    drupal_set_title(t('Edit Tag'));
    $tid = (int) arg(4);
    $term_data['term'] = taxonomy_get_term($tid);
    $term_data['synonym'] = taxonomy_get_synonyms($tid);
  } else {
    drupal_set_title(t('Add Tag'));
  }

  $form['name']  = array(
    '#type' => 'textfield',
    '#title' => t('Tag name'),
    '#default_value' => (!empty($term_data) ? $term_data['term']->name : ''),
    '#size' => 60,
    '#maxlength' => 128,
    '#required' => TRUE,
  );
  $form['synonyms'] = array(
      '#type' => 'textarea',
      '#title' => t('Synonyms'),
      '#description' => t('Synonyms of this term, list of synonyms with comma separated or entered in new line.'),
      '#default_value' => (!empty($term_data) ? implode(", ", $term_data['synonym']) : ''),
      '#size' => 60,
      '#rows' => 5,
      '#cols' => 20,
    );
  if(!empty($term_data)) {
    $form['tid'] = array(
      '#type' => 'hidden',
      '#value' => $term_data['term']->tid,
    );
  }

  $form['save'] = array(
    '#type' => 'submit',
    '#value' => (!empty($term_data) ? t('Update') : t('Save')),
  );

  $form['cancel'] = array(
    '#type' => 'button',
    '#value' => t('Cancel'),
  );

  return $form;
}

/**
 * validate function for admin_edit_tag()
 * @param array $form
 * @param array $form_state
 */
function admin_edit_tag_validate($form, &$form_state) {

  $errors = form_get_errors() != FALSE ? form_get_errors() : array();
  $drupal_errors = drupal_get_messages('error') != FALSE ? drupal_get_messages('error') : array();
  $edit_tid = (!empty($form_state['values']['tid']) ? $form_state['values']['tid'] : NULL);
  if($form_state['values']['op'] == 'Cancel') {
    $from_state['rebuild'] = TRUE;
    drupal_goto('admin/taxonomy/tag'); //Ignore all the checks if Cancel is pressed
  }
    if(trim($form_state['values']['name']) == '') {
      drupal_set_message(t('The tag name is required.'), 'error');
    }
    if($form_state['values']['name'] != '') {
      if($edit_tid) {
        $term = taxonomy_get_term($edit_tid);
        if(strtolower($term->name) != strtolower(trim($form_state['values']['name']))) {
          $exists = taxonomy_get_term_by_name(trim($form_state['values']['name']));
          if($exists) {
            form_set_error('name', t('A category/sub category or tag already exists with the same name.'));
          }
          $tag_as_synonyms = db_result(db_query("SELECT COUNT(name) FROM {term_synonym} WHERE tid <> %d AND name = '%s'", $edit_tid, trim($form_state['values']['name'])));
          if($tag_as_synonyms > 0) {
            form_set_error('name', t('Tag !tag already exists as a synonym to another tag.', array(
              '!tag' => trim($form_state['values']['name']),
            )));
          }
        }
      } elseif($edit_tid === NULL) {
        $exists = taxonomy_get_term_by_name(trim($form_state['values']['name']));
        if($exists) {
          form_set_error('name', t('A category/sub category or tag already exists with the same name.'));
        }
        $tag_as_synonyms = db_result(db_query("SELECT COUNT(name) FROM {term_synonym} WHERE name = '%s'", trim($form_state['values']['name'])));
        if($tag_as_synonyms > 0) {
          form_set_error('name', t('Tag !tag already exists as a synonym to another tag.', array(
            '!tag' => trim($form_state['values']['name']),
          )));
        }
      }
    }
    if(!empty($form_state['values']['synonyms'])) {
      $delimiters = array(",", "\n");
      $synonyms_array = str_replace($delimiters, $delimiters[0], $form_state['values']['synonyms']);
	    $synonyms = explode(",", html_entity_decode(filter_xss($synonyms_array)));
      $synonyms = array_filter($synonyms);
      $synonyms = array_filter(array_map('trim', $synonyms));

      //Check if the synonyms exists as tags or as other synonyms to different tag
      $tag_exists = array();
      foreach($synonyms as $synonym) {
		$synonyms_lower[] = strtolower($synonym);
        //If tag name exists
        if(taxonomy_get_term_by_name($synonym)) {
          $tag_exists[] = $synonym;
        }
      }

      $tag_exists = array_unique($tag_exists);
      if(!empty($tag_exists)) {
        form_set_error('synonyms', t('@tag @ext with the given synonym @names !sn', array(
          '@tag' => format_plural(count($tag_exists), 'Tag', 'Tags'),
          '@ext' => format_plural(count($tag_exists), 'exists', 'exist'),
          '@names' => format_plural(count($tag_exists), 'name', 'names'),
          '!sn' => implode(", ", $tag_exists),
          )));
      }
	  elseif(in_array(strtolower(trim($form_state['values']['name'])), $synonyms_lower)) {
	    form_set_error('synonyms', t('The Tag has the same name as one of its synonyms.'));
	  }
      //Check for synonym exists
      if($edit_tid) {
        //The term is in the edit mode
        $arguments = array($form_state['values']['tid']);
        $arguments = array_merge($arguments, $synonyms);
        $placeholders = db_placeholders($synonyms, 'varchar');
        $synonym_exists_query = "SELECT TS.name FROM {term_synonym} AS TS WHERE TS.tid <> %d AND TS.name IN ($placeholders)";
      } else {
        $placeholders = db_placeholders($synonyms, 'varchar');
        $arguments = $synonyms;
        $synonym_exists_query = "SELECT TS.name FROM {term_synonym} AS TS WHERE TS.name IN ($placeholders)";
      }
      $synonym_exists_rs = db_query($synonym_exists_query, $arguments);
      if($synonym_exists_rs->num_rows > 0) {
        while($synonym_exists_data = db_fetch_array($synonym_exists_rs)) {
          $synonym_exists[] = $synonym_exists_data['name'];
        }
        $synonym_exists = array_unique($synonym_exists);
        form_set_error('', t('@sn !name already @ext as a synonym to another tag or as a tag', array(
          '@sn' => format_plural(count($synonym_exists), 'Synonym', 'Synonyms'),
          '@ext' => format_plural(count($synonym_exists), 'exists', 'exist'),
          '!name' => implode(", ", $synonym_exists),
        )));
      }
    }
  //} //Else post the form for validation
}

/*
 * Submit function for admin_edit_tag()
 * @param array $form
 * @param array $form_state
 */
function admin_edit_tag_submit($form, &$form_state) {
  $synonyms = explode(",", html_entity_decode(filter_xss($form_state['values']['synonyms'])));
  $synonyms = array_filter($synonyms);
  $synonyms = array_filter(array_map('trim', $synonyms));

  foreach($synonyms as $key => $str) {
    $synonyms_lower1[$key] = strtolower($str);
  }
  $synonyms_key = array();
  $synonyms_key = array_unique($synonyms_lower1);
  foreach($synonyms_key as $key => $value) {
    $unique_synonyms[] = $synonyms[$key];
  }
  $audit_synonyms = implode("; ", $unique_synonyms);
  $synonyms = implode("\r\n", $unique_synonyms);

  $tag = array();
  $old_synonyms = array();
  if(isset($form_state['values']['tid'])) {
    //This is in edit mode

    //preparing data for audit data
    $old_synonyms = taxonomy_get_synonyms($form_state['values']['tid']);
    $old_synonyms = implode("; ", $old_synonyms);

    $old_tag_data = taxonomy_get_term($form_state['values']['tid']);
    $old_tag_name = $old_tag_data->name;
    //ends...

    $success_message = t('The Tag is successfully updated.');
    $tag += array('tid' => $form_state['values']['tid']);
  } else {
    //In Add Mode
    $success_message = t('The tag @tn is saved successfully.', array('@tn' => $form_state['values']['name']));
  }

  $tag += array(
    'vid' => TAGS_VOCABULARY_ID,
    'name' => html_entity_decode(filter_xss($form_state['values']['name'])),
    'synonyms' => $synonyms,
    'relations' => array(),
  );

  if(taxonomy_save_term($tag)) {
    if(isset($form_state['values']['tid'])) {
      //@todo: Audit Trail for Edit Term
      $record_name = 'Tag';
      $audit_type = 'CHG';
      $old_values = array();
      $new_values = array();
      if($old_tag_name != $form_state['values']['name']){
        $new_values['tag_name'] = $form_state['values']['name'];
        $old_values['tag_name'] = $old_tag_name;
      }
      if($synonyms != $old_synonyms){
        $tag_item_name = $old_tag_name;
        $new_values['synonyms'] = $audit_synonyms;
        $old_values['synonyms'] = $old_synonyms;
      }

      if(function_exists('taxonomy_audit_trail_record_process')){
        taxonomy_audit_trail_record_process($record_name, $audit_type, $old_values, $new_values, $tag_item_name);
      }
      //audit trail ends
    } else {
      //@todo: Audit trail for Add Term
      $record_name = 'Tag';
      $audit_type = 'ADD';
      $new_values = array();
      $new_values['tag_name'] = $form_state['values']['name'];
      if($synonyms != ''){
        $new_values['synonyms'] = $synonyms;
      }
      $old_values = NULL;
      if(function_exists('taxonomy_audit_trail_record_process')){
        taxonomy_audit_trail_record_process($record_name, $audit_type, $old_values, $new_values);
      }
      //audit trail ends
    }
    drupal_set_message($success_message);
    drupal_goto('admin/taxonomy/tag');

  }
}

/**
 * Function to export the terms
 * @param  string $type
 * @return string
 */
function vocabulary_term_export($type) {
  $generate_report = FALSE;
  $csv = NULL;
  switch($type) {
    case "tag" :
      //Get the Tag id and then load all the Terms data related to it
      $term_tree = taxonomy_get_tree(TAGS_VOCABULARY_ID, 0, -1, NULL);
      if(!empty($term_tree)) {
        $generate_report = TRUE;
        foreach($term_tree as $key => $term) {
          $vocab_alter_dates = vocabulary_term_alter_dates($term->tid);
          $term_tree[$key]->synonym = implode('; ', taxonomy_get_synonyms($term->tid));
          $term_tree[$key]->total_nodes = taxonomy_term_count_nodes($term->tid);
          $term_tree[$key]->community_nodes = community_node_count($term->tid);
          $term_tree[$key]->created = $vocab_alter_dates['created'];
          $term_tree[$key]->updated = $vocab_alter_dates['updated'];
        }
        //Create the CSV Record
        unset($key);
        unset($term);
        $csv_header = array(array('Name of the tag', 'List of synonyms', 'Date of creation', 'Date of last modification', 'Number of content associated with each tag', 'Number of community content associated with each tag'));
        foreach($term_tree as $key => $term) {
          $csv_data[] =array(
            $term->name,
            (!empty($term->synonym) ? $term->synonym : t('N/A')),
            (!empty($term->created) ? format_date($term->created, 'custom', 'Y-m-d H:i:s') : t('N/A')),
            (!empty($term->updated) ? format_date($term->updated, 'custom', 'Y-m-d H:i:s') : t('N/A')),
            (!empty($term->total_nodes) ? $term->total_nodes : 0),
            (!empty($term->community_nodes) ? $term->community_nodes : t('N/A')),
          );
        }
        $csv = array_merge($csv_header, $csv_data); //Merging the Header & Body
      }
    break;

    case "category" :
      $term_tree = taxonomy_get_tree(CATEGORY_VOCABULARY_ID, 0, -1, NULL);
      if(!empty($term_tree)) {
        $generate_report = TRUE;
        foreach($term_tree as $key => $term) {
		  $node_count = term_node_count_exclude_child($term->tid);
          $vocab_alter_dates = vocabulary_term_alter_dates($term->tid);
          $parents = taxonomy_get_parents_all($term->tid);
          $parent_count = count($parents);
          if($parent_count == 1) {
            $parents_str = 'N/A';
          } else {
            $parents_str = '';

            unset($parents[0]);

            $parents = array_reverse($parents, true);
            foreach($parents as $pid => $term) {
              $parents_str .= $term->name . ' > ';
            }
          }
          $term_tree[$key]->parent = substr($parents_str, 0, -2);
          $term_tree[$key]->total_nodes = $node_count;
          $term_tree[$key]->community_nodes = community_node_count($term->tid);
          $term_tree[$key]->created = $vocab_alter_dates['created'];
          $term_tree[$key]->updated = $vocab_alter_dates['updated'];
        }

        //Create the CSV Record
        unset($key);
        unset($term);
        $csv_header = array(array('Category Name', 'Parent Category', 'Date of creation', 'Date of last modification', 'Number of content associated with each category', 'Number of community content associated with each category'));
        foreach($term_tree as $key => $term) {
          $csv_data[] =array(
            $term->name,
            (!empty($term->parent) ? $term->parent : t('N/A')),
            (!empty($term->created) ? format_date($term->created, 'custom', 'Y-m-d H:i:s') : t('N/A')),
            (!empty($term->updated) ? format_date($term->updated, 'custom', 'Y-m-d H:i:s') : t('N/A')),
            (!empty($term->total_nodes) ? $term->total_nodes : 0),
            (!empty($term->community_nodes) ? $term->community_nodes : t('N/A')),
          );
        }
        $csv = array_merge($csv_header, $csv_data); //Merging the Header & Body
      }
    break;
  }

  if($generate_report === TRUE && $csv !== NULL) {
    drupal_set_header("Content-type: text/csv");
    drupal_set_header("Content-Disposition: attachment; filename=" . $type . "_" . date("Y") . ".csv");
    drupal_set_header("Pragma: no-cache");
    drupal_set_header("Expires: 0");
    outputCSV($csv);
    exit();
  } else {
    drupal_set_message(t('Nothing to generate'), 'error');
    drupal_goto("admin/taxonomy/$type");
  }
}

/**
 * Function for count term nodes excluding their children count
 */
function term_node_count_exclude_child($tid){
  $count_sql = "SELECT COUNT(n.nid) AS c FROM {term_node} t
                    INNER JOIN {node} n ON t.vid = n.vid
                    WHERE n.status = 1 AND t.tid = %d " ;
  $node_count = db_result(db_query($count_sql, $tid));
  return $node_count;
}

/**
 * CSV generation function
 * @param  string $data
 * @return string
 */
if(!function_exists('outputCSV')) {
  function outputCSV($data) {
    $outstream = fopen("php://output", "w");
    function __outputCSV(&$vals, $key, $filehandler) {
      fputcsv($filehandler, $vals); // add parameters if you want
    }
    array_walk($data, "__outputCSV", $outstream);
    fclose($outstream);
  }
}



/**
 * Fetch the created & updated date for each Terms
 * @param  int $tid
 * @return array
 */
function vocabulary_term_alter_dates($tid) {
  $dates_query = db_query("SELECT created, updated FROM {vocabulary_timestamps} WHERE tid = %d", $tid);
  $dates_data = db_fetch_object($dates_query);
  return array(
    'created' => (!empty($dates_data->created) ? $dates_data->created : NULL),
    'updated' => (!empty($dates_data->updated) ? $dates_data->updated : NULL),
  );
}


function cet_taxonomy_admin_view_categories() {
  global $pager_page_array, $pager_total, $pager_total_items;
  $form = array();
  //It is assumed that the Category will be pre fixed
  $category_tid = CATEGORY_VOCABULARY_ID;
  $vocabulary = taxonomy_vocabulary_load($category_tid);
  $form = array(
    '#vocabulary' => (array)$vocabulary,
    '#tree' => TRUE,
    '#parent_fields' => FALSE,
  );
  $page = isset($_GET['page']) ? $_GET['page'] : 0;
  $page_increment = variable_get('taxonomy_terms_per_page_admin', 100); //CET_DEFAULT_PER_PAGE_NUM;
  $page_entries = 0;   // Elements shown on this page.
  $before_entries = 0;   // Elements at the root level before this page.
  $after_entries = 0;   // Elements at the root level after this page.
  $root_entries = 0;   // Elements at the root level on this page.

  $back_peddle = NULL;
  $forward_peddle = 0;
  // An array of the terms to be displayed on this page.
  $current_page = array();

  if ($vocabulary->tags) {
    // We are not calling taxonomy_get_tree because that might fail with a big
    // number of tags in the freetagging vocabulary.
    $results = pager_query(db_rewrite_sql('SELECT t.*, h.parent FROM {term_data} t INNER JOIN {term_hierarchy} h ON t.tid = h.tid WHERE t.vid = %d ORDER BY weight, name', 't', 'tid'), $page_increment, 0, NULL, $vocabulary->vid);
    $total_entries = db_query(db_rewrite_sql('SELECT count(*) FROM {term_data} t INNER JOIN {term_hierarchy} h ON t.tid = h.tid WHERE t.vid = %d', 't', 'tid'), $page_increment, 0, NULL, $vocabulary->vid);
    while ($term = db_fetch_object($results)) {
      $key = 'tid:'. $term->tid .':0';
      $current_page[$key] = $term;
      $page_entries++;
    }
  } else {
    $term_deltas = array();
    $tree = taxonomy_get_tree($vocabulary->vid);
    $term = current($tree);
    do {

      // In case this tree is completely empty.
      if (empty($term)) {
        break;
      }

      // Count entries before the current page.
      if ($page && ($page * $page_increment) > $before_entries && !isset($back_peddle)) {
        $before_entries++;
        continue;
      }
      // Count entries after the current page.
      elseif ($page_entries > $page_increment && isset($complete_tree)) {
        $after_entries++;
        continue;
      }

      // Do not let a term start the page that is not at the root.
      if (isset($term->depth) && ($term->depth > 0) && !isset($back_peddle)) {
        $back_peddle = 0;
        while ($pterm = prev($tree)) {
          $before_entries--;
          $back_peddle++;
          if ($pterm->depth == 0) {
            prev($tree);
            continue 2; // Jump back to the start of the root level parent.
          }
        }
      }
      $back_peddle = isset($back_peddle) ? $back_peddle : 0;

      // Continue rendering the tree until we reach the a new root item.
      if ($page_entries >= $page_increment + $back_peddle + 1 && $term->depth == 0 && $root_entries > 1) {
        $complete_tree = TRUE;
        // This new item at the root level is the first item on the next page.
        $after_entries++;
        continue;
      }
      if ($page_entries >= $page_increment + $back_peddle) {
        $forward_peddle++;
      }

      // Finally, if we've gotten down this far, we're rendering a term on this page.
      $page_entries++;
      $term_deltas[$term->tid] = isset($term_deltas[$term->tid]) ? $term_deltas[$term->tid] + 1 : 0;
      $key = 'tid:'. $term->tid .':'. $term_deltas[$term->tid];

      // Keep track of the first term displayed on this page.
      if ($page_entries == 1) {
        $form['#first_tid'] = $term->tid;
      }
      // Keep a variable to make sure at least 2 root elements are displayed.
      if ($term->parents[0] == 0) {
        $root_entries++;
      }
      $current_page[$key] = $term;
    } while ($term = next($tree));

    // Because we didn't use a pager query, set the necessary pager variables.
    $total_entries = $before_entries + $page_entries + $after_entries;
    $pager_total_items[0] = $total_entries;
    $pager_page_array[0] = $page;
    $pager_total[0] = ceil($total_entries / $page_increment);
  }
  //Get the number of content associated with each tag
  foreach($current_page as $term_key => $term_data) {
    $tid_set[] = $term_data->tid;
  }
  if(!empty($tid_set)) {
    $term_placeholders = db_placeholders($tid_set, 'int');
    $contents_tagged_query = db_query(db_rewrite_sql("SELECT t.tid, COUNT(n.nid) AS c FROM {term_node} t INNER JOIN {node} n ON t.vid = n.vid WHERE n.status = 1 AND t.tid IN ($term_placeholders) GROUP BY t.tid"), $tid_set);
    while($contents_tagged_data = db_fetch_object($contents_tagged_query)) {
      $tagged_terms[$contents_tagged_data->tid] = $contents_tagged_data->c;
    }
  }
  unset($term_key);
  unset($term_data);
  //Merge the array based on the content count
  foreach($current_page as $term_key => $term_data) {
    $current_page[$term_key]->content_associated = 0;
    if(isset($tagged_terms[$term_data->tid]) && $tagged_terms[$term_data->tid] > 0) {
      $current_page[$term_key]->content_associated = $tagged_terms[$term_data->tid];
    }
  }

  foreach ($current_page as $key => $term) {
    // Save the term for the current page so we don't have to load it a second time.
    $form[$key]['#term'] = (array) $term;

    if (isset($term->parents)) {
      $form[$key]['#term']['parent'] = $term->parent = $term->parents[0];
      unset($form[$key]['#term']['parents'], $term->parents);
    }

      $form[$key]['view'] = array('#value' => check_plain($term->name));


    if($term->content_associated > 0) {
      $content_associated = l($term->content_associated, 'taxonomy/term/' . $term->tid);
    } else {
      $content_associated = $term->content_associated;
    }
    $form[$key]['nodes_associated'] = array('#value' => $content_associated);
    if (!$vocabulary->tags && $vocabulary->hierarchy < 2 && count($tree) > 1) {
      $form['#parent_fields'] = TRUE;
      $form[$key]['tid'] = array(
        '#type' => 'hidden',
        '#value' => $term->tid
      );
      $form[$key]['parent'] = array(
        '#type' => 'hidden',
        // Yes, default_value on a hidden. It needs to be changeable by the javascript.
        '#default_value' => $term->parent,
      );
      $form[$key]['depth'] = array(
        '#type' => 'hidden',
        // Same as above, the depth is modified by javascript, so it's a default_value.
        '#default_value' => $term->depth,
      );
      $form[$key]['node_associated'] = array(
        '#type' => 'hidden',
        '#value' => $term->content_associated,
      );
      //Create an array that will store the pre saved form state
      $term_skeliton[$key] = array(
        'tid' => $term->tid,
        'parent' => $term->parent,
        'depth' => $term->depth,
        'node_associated' => $term->content_associated,
      );
    }
    $form[$key]['edit'] = array('#value' => l(t('Edit'), "admin/taxonomy/category/edit/$term->tid", array('query' => drupal_get_destination())));

  }
  $form['#total_entries'] = $total_entries;
  $form['#page_increment'] = $page_increment;
  $form['#page_entries'] = $page_entries;
  $form['#back_peddle'] = $back_peddle;
  $form['#forward_peddle'] = $forward_peddle;
  $form['#empty_text'] = t('No categories available.');
  $form['#term_skliton'] = $term_skeliton;

  if (!$vocabulary->tags && $vocabulary->hierarchy < 2 && count($tree) > 1) {
    $form['submit'] = array(
      '#type' => 'submit',
      '#value' => t('Save')
    );
    $form['destination'] = array(
      '#type' => 'hidden',
      '#value' => $_GET['q'] . (isset($_GET['page']) ? '?page='. $_GET['page'] : '')
    );
  }

  $form['#theme'] = 'category_overview_terms';

  return $form;
}

/**
 * Validation function for the Category listing page
 * @param  array $form
 * @param  array $form_state
 * @return void
 */
function cet_taxonomy_admin_view_categories_validate($form, &$form_state) {
  $order = array_flip(array_keys($form['#post'])); // Get the $_POST order.
  $terms_changed_order = $form_state['values'] = array_merge($order, $form_state['values']); // Update our original form with the new order.
  
  //Check if any term has the depth of 3
  $depth_restrict = array();
  $terms_changed_order = array_filter($terms_changed_order, 'is_array');
  foreach ($terms_changed_order as $tid => $values) {
    if($values['depth'] >= VOCAB_CATEGORY_MAX_DEPTH) {
      $depth_restrict[] = $values['tid'];
    }
  }
  if(!empty($depth_restrict)) {
    //Set the error for the nesting level
    form_set_error('', t('Categories should not have a third level of hierarchy.'));
  }
}

/**
 * Submit function for the Category listing page to change the hierarchy
 * @param  array $form
 * @param  array $form_state
 * @return void
 */
function cet_taxonomy_admin_view_categories_submit($form, &$form_state) {
  $order = array_flip(array_keys($form['#post'])); // Get the $_POST order.

  $terms_changed_order = $form_state['values'] = array_merge($order, $form_state['values']); // Update our original form with the new order.

  //@todo: Audit Trail for Edit Category
  $term_order_diff = array_diff_values($terms_changed_order, $form['#term_skliton']);
  $term_order_diff = array_filter($term_order_diff, 'is_array');
  foreach ($term_order_diff as $diff_key => $diff_value) {
    $record_name = 'Category';
    $audit_type = 'CHG';
    $old_values = array();
    $new_values = array();
    $cat_data = taxonomy_get_term($diff_value['tid']);
    $cat_item_name = $cat_data->name;
    $old_parent_data = taxonomy_get_parents($diff_value['tid'], 'tid');
    foreach ($old_parent_data as $old_key => $old_value) {
      $old_values['parent'] = $old_value->name;
    }
    $parent_data = taxonomy_get_term($diff_value['parent']);
    $new_values['parent'] = $parent_data->name;

    if(function_exists('taxonomy_audit_trail_record_process')){
      taxonomy_audit_trail_record_process($record_name, $audit_type, $old_values, $new_values, $cat_item_name);
    }
  }

  //audit trail ends
  $vocabulary = $form['#vocabulary'];
  $hierarchy = 0; // Update the current hierarchy type as we go.

  $changed_terms = array();
  $tree = taxonomy_get_tree($vocabulary['vid']);

  if (empty($tree)) {
    return;
  }

  // Build a list of all terms that need to be updated on previous pages.
  $weight = 0;
  $term = (array)$tree[0];
  while ($term['tid'] != $form['#first_tid']) {
    if ($term['parents'][0] == 0 && $term['weight'] != $weight) {
      $term['parent'] = $term['parents'][0];
      $term['weight'] = $weight;
      $changed_terms[$term['tid']] = $term;
    }
    $weight++;
    $hierarchy = $term['parents'][0] != 0 ? 1 : $hierarchy;
    $term = (array)$tree[$weight];
  }

  // Renumber the current page weights and assign any new parents.
  $level_weights = array();
  foreach ($form_state['values'] as $tid => $values) {
    if (isset($form[$tid]['#term'])) {
      $term = $form[$tid]['#term'];
      // Give terms at the root level a weight in sequence with terms on previous pages.
      if ($values['parent'] == 0 && $term['weight'] != $weight) {
        $term['weight'] = $weight;
        $changed_terms[$term['tid']] = $term;
      }
      // Terms not at the root level can safely start from 0 because they're all on this page.
      elseif ($values['parent'] > 0) {
        $level_weights[$values['parent']] = isset($level_weights[$values['parent']]) ? $level_weights[$values['parent']] + 1 : 0;
        if ($level_weights[$values['parent']] != $term['weight']) {
          $term['weight'] = $level_weights[$values['parent']];
          $changed_terms[$term['tid']] = $term;
        }
      }
      // Update any changed parents.
      if ($values['parent'] != $term['parent']) {
        $term['parent'] = $values['parent'];
        $changed_terms[$term['tid']] = $term;

      }
      $hierarchy = $term['parent'] != 0 ? 1 : $hierarchy;

      $weight++;
    }
  }

  // Build a list of all terms that need to be updated on following pages.
  for ($weight; $weight < count($tree); $weight++) {
    $term = (array)$tree[$weight];
    if ($term['parents'][0] == 0 && $term['weight'] != $weight) {
      $term['parent'] = $term['parents'][0];
      $term['weight'] = $weight;
      $changed_terms[$term['tid']] = $term;
    }
    $hierarchy = $term['parents'][0] != 0 ? 1 : $hierarchy;
  }

  // Save all updated terms.
  foreach ($changed_terms as $term) {
    taxonomy_save_term($term);
  }

  // Update the vocabulary hierarchy to flat or single hierarchy.
  if ($vocabulary['hierarchy'] != $hierarchy) {
    $vocabulary['hierarchy'] = $hierarchy;
    taxonomy_save_vocabulary($vocabulary);
  }
  drupal_set_message(t('You will need to re-index the content for this change to take effect.'), 'warning');
}

/**
 * Theme function for the Category listing page
 * @param  array $form
 * @return array
 */
function theme_category_overview_terms($form) {
  $total_entries = $form['#total_entries'];
  $page_increment = $form['#page_increment'];
  $page_entries = $form['#page_entries'];
  $back_peddle = $form['#back_peddle'];
  $forward_peddle = $form['#forward_peddle'];
  // Add drag and drop if parent fields are present in the form.
  if ($form['#parent_fields']) {
    drupal_add_tabledrag('taxonomy', 'match', 'parent', 'term-parent', 'term-parent', 'term-id', FALSE);
    drupal_add_tabledrag('taxonomy', 'depth', 'group', 'term-depth', NULL, NULL, FALSE);
    drupal_add_js(drupal_get_path('module', 'taxonomy') .'/taxonomy.js');
    drupal_add_js(drupal_get_path('module', 'cet_taxonomy_admin') . '/js/cet_taxonomy_admin.js');
    drupal_add_js(array('taxonomy' => array('backPeddle' => $back_peddle, 'forwardPeddle' => $forward_peddle)), 'setting');
    drupal_add_css(drupal_get_path('module', 'taxonomy') .'/taxonomy.css');
  }

  $errors = form_get_errors() != FALSE ? form_get_errors() : array();
  $rows = array();
  foreach (element_children($form) as $key) {
    if (isset($form[$key]['#term'])) {
      $term = &$form[$key];

      $row = array();
      $row[] = (isset($term['#term']['depth']) && $term['#term']['depth'] > 0 ? theme('indentation', $term['#term']['depth']) : '') . drupal_render($term['view']);
      if ($form['#parent_fields']) {
        $term['tid']['#attributes']['class'] = 'term-id';
        $term['parent']['#attributes']['class'] = 'term-parent';
        $term['depth']['#attributes']['class'] = 'term-depth';
        $row[0] .= drupal_render($term['parent']) . drupal_render($term['tid']) . drupal_render($term['depth']);
      }
      $row[] = drupal_render($term['nodes_associated']);
      $row[] = drupal_render($term['edit']);
      $row = array('data' => $row);
      $rows[$key] = $row;
    }
  }

  // Add necessary classes to rows.
  $row_position = 0;
  foreach ($rows as $key => $row) {
    $classes = array();
    if (isset($form['#parent_fields'])) {
      $classes[] = 'draggable';
    }

    // Add classes that mark which terms belong to previous and next pages.
    if ($row_position < $back_peddle || $row_position >= $page_entries - $forward_peddle) {
      $classes[] = 'taxonomy-term-preview';
    }

    if ($row_position !== 0 && $row_position !== count($rows) - 1) {
      if ($row_position == $back_peddle - 1 || $row_position == $page_entries - $forward_peddle - 1) {
        $classes[] = 'taxonomy-term-divider-top';
      }
      elseif ($row_position == $back_peddle || $row_position == $page_entries - $forward_peddle) {
        $classes[] = 'taxonomy-term-divider-bottom';
      }
    }

    // Add an error class if this row contains a form error.
    foreach ($errors as $error_key => $error) {
      if (strpos($error_key, $key) === 0) {
        $classes[] = 'error';
      }
    }
    $rows[$key]['class'] = implode(' ', $classes);
    $row_position++;
  }

  if (empty($rows)) {
    $rows[] = array(array('data' => $form['#empty_text'], 'colspan' => '3'));
  }

  $header = array(t('Name'), t('Number of content associated'), t('Actions'));
  $element['#value'] = '<div><strong>Number of categories:</strong> ' . $total_entries .'</div>';
  $output = theme('item', $element);
  $output .= theme('table', $header, $rows, array('id' => 'taxonomy'));
  $output .= drupal_render($form);
  $output .= theme('pager', NULL, $page_increment);

  return $output;
}



function admin_edit_category() {
  $form = array();
  $op = arg(3);
  $category_vocab = CATEGORY_VOCABULARY_ID;
  $term = NULL;
  if ($op == 'add') {
    drupal_set_title(t('Add Category/Sub Category'));
    $exclude = array();
    $parent = array();
    $button_value = t('Save');
  }
  elseif ($op == 'edit') {
    $tid = (int) arg(4);

    // CLP-2798 : Preventing Carrier term edit.
    if ($tid == VOCAB_CATEGORY_CAREERS_TERM_ID) {
      drupal_access_denied();
      module_invoke_all('exit');
      exit();
    }

    drupal_set_title(t('Edit Category/Sub Category'));
    $term = taxonomy_get_term($tid);
    $parent = array_keys(taxonomy_get_parents($tid));
    $children = taxonomy_get_tree($category_vocab, $tid);
    foreach ($children as $child) {
      $exclude[] = $child->tid;
    }
    $exclude[] = $tid;
    // Set the hidden tag id.
    $form['tid'] = array(
      '#type' => 'hidden',
      '#default_value' => $tid,
    );
    $button_value = t('Update');
  }
  $form['category_name'] = array(
    '#type' => 'textfield',
    '#title' => t('Name'),
    '#default_value' => (isset($term) ? $term->name : NULL),
    '#required' => TRUE,
  );


  // A term can't be the child of itself, nor of its children.
  foreach ($children as $child) {
    $exclude[] = $child->tid;
  }
  $exclude[] = $child->tid;

  $form['parent'] = _taxonomy_category_select(t('Parent category'), 'parent', $parent, $category_vocab, t('Parent terms') .'.', FALSE, '<'. t('root') .'>', TRUE, NULL, $exclude);
  $form['#term']['parent'] = $parent;
  $form['category_submit'] = array(
    '#type' => 'submit',
    '#value' => $button_value,
  );
  $form['category_cancel'] = array(
    '#type' => 'submit',
    '#value' => t('Cancel'),
  );
  return $form;
}


function admin_edit_category_validate($form, &$form_state) {
  $errors = form_get_errors() != FALSE ? form_get_errors() : array();
  $drupal_errors = drupal_get_messages('error') != FALSE ? drupal_get_messages('error') : array();
  $edit_tid = (!empty($form_state['values']['tid']) ? $form_state['values']['tid'] : NULL);
  //Cancel Link access
  if($form_state['values']['op'] == 'Cancel') {
    $form_state['rebuild'] = TRUE;
    drupal_goto('admin/taxonomy/category');
  }
  //Category Name Validations
  if (trim($form_state['values']['category_name']) == '') {
    drupal_set_message(t('Category/sub category name cannot be blank'), 'error');
  } elseif(trim($form_state['values']['category_name']) != '') {
    if($edit_tid) {
      $term = taxonomy_get_term($edit_tid);
      if(strtolower($term->name) != strtolower(trim($form_state['values']['category_name']))) {
        $exists = taxonomy_get_term_by_name(trim($form_state['values']['category_name']));
        if($exists) {
          form_set_error('category_name', t('A category/sub category or tag already exists with the same name.'));
        }
      }
    } elseif($edit_tid === NULL) {
      $exists = taxonomy_get_term_by_name(trim($form_state['values']['category_name']));
      if($exists) {
        form_set_error('category_name', t('A category/sub category or tag already exists with the same name.'));
      }
    }
  } //Category Name validation ends

  //Check for the depth of the categories
  $selected_parent = $form_state['values']['parent'];
  if($selected_parent != 0) {
    //This is not root category
    //Get the Parent's depth and restrict the user
    $selected_parent_depth = term_depth($selected_parent);
    if($selected_parent_depth >= VOCAB_CATEGORY_MAX_DEPTH) {
      form_set_error('parent', t('Categories should not have beyond third level of hierarchy.'));
    }
  }
}


function admin_edit_category_submit($form, &$form_state) {
  if(!empty($form_state['values']['category_submit'])) {
    //If the submit button is pressed
    $parent_id =
    $category = array();
    if(isset($form_state['values']['tid'])) {
      //This is in edit mode

      //preparing data for audit data
      $old_parent_data = taxonomy_get_parents($form_state['values']['tid'], 'tid');
      foreach ($old_parent_data as $p_key => $p_value) {
        $old_parent_name = $p_value->name;
        $old_parent_id = $p_value->tid;
      }

      $old_cat_data = taxonomy_get_term($form_state['values']['tid']);

      $old_cat_name = $old_cat_data->name;

      //ends...
      $success_message = t('The update is successful.');
      $category += array('tid' => $form_state['values']['tid']);
    } else {
      //In Add Mode
      $success_message = t('The category @cn is saved successfully.', array('@cn' => $form_state['values']['name']));
    }
    $category += array(
      'vid' => CATEGORY_VOCABULARY_ID,
      'name' => filter_xss($form_state['values']['category_name']),
      'relations' => array(),
      'parent' => $form_state['values']['parent'],
    );
    if(taxonomy_save_term($category)) {
      if(isset($form_state['values']['tid'])) {
        drupal_set_message($success_message);
        //@todo: Audit Trail for Edit Category
        $record_name = 'Category';
        $audit_type = 'CHG';
        $old_values = array();
        $new_values = array();
        if($old_cat_name != $form_state['values']['category_name']){
          $new_values['category_name'] = $form_state['values']['category_name'];
          $old_values['category_name'] = $old_cat_name;
        }
        if($old_parent_id != $form_state['values']['parent']){
          $new_parent_data = taxonomy_get_parents($form_state['values']['tid'], 'tid');
          foreach ($new_parent_data as $n_key => $n_value) {
            $new_parent_name = $n_value->name;
          }
          $cat_item_name = $old_cat_name;
          $new_values['parent'] = $new_parent_name;
          $old_values['parent'] = $old_parent_name;
        }

        if(function_exists('taxonomy_audit_trail_record_process')){
          taxonomy_audit_trail_record_process($record_name, $audit_type, $old_values, $new_values, $cat_item_name);
        }
        //audit trail ends
      } else {
        //@todo: Audit trail for Add Category
          $record_name = 'Category';
          $audit_type = 'ADD';
          $new_values = array();
          $new_values['category_name'] = $form_state['values']['category_name'];
          $parent_data = taxonomy_get_term($form_state['values']['parent']);

          $new_values['parent'] = $parent_data->name;

          $old_values = NULL;
          if(function_exists('taxonomy_audit_trail_record_process')){
            taxonomy_audit_trail_record_process($record_name, $audit_type, $old_values, $new_values);
          }
          drupal_set_message($success_message);
          drupal_goto('admin/taxonomy/category');

        }
    }
  }
}

/**
 * To diff between n-dimensional array
 * @see http://www.php.net/manual/en/function.array-diff-assoc.php#75269
 * @param array $tab1
 * @param  array $tab2
 * @return array
 */
function array_diff_values($tab1, $tab2) {
  $result = array();
  foreach($tab1 as $values) if(! in_array($values, $tab2)) $result[] = $values;
  return $result;
}


/**
 * Clonned modified function from _taxonomy_term_select()
 * @see _taxonomy_term_select()
 * @param $title
 *   The title of the vocabulary. This MUST be sanitized by the caller.
 * @param $name
 *   Ignored.
 * @param $value
 *   The currently selected terms from this vocabulary, if any.
 * @param $vocabulary_id
 *   The vocabulary ID to build the form element for.
 * @param $description
 *   Help text for the form element. This MUST be sanitized by the caller.
 * @param $multiple
 *   Boolean to control if the form should use a single or multiple select.
 * @param $blank
 *   Optional form choice to use when no value has been selected.
 * @param $exclude
 *   Optional array of term ids to exclude in the selector.
 * @return
 *   A FAPI form array to select terms from the given vocabulary.
 *
 */
function _taxonomy_category_select($title, $name, $value, $vocabulary_id, $description, $multiple, $blank, $required = TRUE, $weight = NULL, $exclude = array()) {
  $tree = taxonomy_get_tree($vocabulary_id);
  $options = array();

  if ($blank) {
    $options[0] = $blank;
  }
  if ($tree) {
    foreach ($tree as $term) {
      if (!in_array($term->tid, $exclude)) {
        $choice = new stdClass();
        $choice->option = array($term->tid => str_repeat('-', $term->depth) . $term->name);
        $options[] = $choice;
      }
    }
  }
  $box = array(
    '#type' => 'select',
    '#title' => $title,
    '#default_value' => $value,
    '#options' => $options,
    '#description' => $description,
    '#multiple' => $multiple,
    '#required' => $required,
    '#size' => $multiple ? min(9, count($options)) : 0,
    '#theme' => 'taxonomy_term_select',
  );
  if($weight !== NULL) {
    $box += array(
      '#weight' => $weight,
    );
  }
  return $box;
}

function term_depth($tid) {
  $parent = db_result(db_query("SELECT parent FROM {term_hierarchy} WHERE tid=%d", $tid));
  if($parent == 0) {
    return 1;
  }else  {
    return (1 + term_depth($parent));
  }
}

/**
 * function for audit trail record insert into DB
 */
function taxonomy_audit_trail_record_process($record_name = NULL, $audit_type = NULL, $old_values = NULL, $new_values = NULL, $item_name = NULL){

  global $user;
  $user_id = $user->uid;
  profile_load_profile($user);
  $user_name = $user->profile_first_name . ' ' . $user->profile_last_name;

  //for tag operations...
  switch($record_name) {
    case 'Tag':

      switch($audit_type) {
        case 'DEL':
          $old_values = serialize($old_values);
          $field_name = NULL;
          $audit_msg = t('Tag has been removed.');
          $audit_args = array($user_id, $user_name, $audit_type, $record_name, $field_name, $old_values, $new_values, time(), $audit_msg);
          db_query("INSERT INTO {taxonomy_audit_trail}
                    (user_id, user_name, audit_type, record_name, field_name, old_value, new_value, record_inserted_on, audit_message)
                VALUES (%d, '%s', '%s', '%s', '%s', '%s', '%s', %d, '%s') ", $audit_args);
        break;

        case 'ADD' :
          $tag_name = $new_values['tag_name'];
          if(array_key_exists('synonyms', $new_values)){
            $field_name = t('Name, Synonym');
          }else{
            $field_name = t('Name');
          }
          $new_values = serialize($new_values);
          $audit_msg = t('Tag !tag_name has been added.', array('!tag_name' => $tag_name));
          $audit_args = array($user_id, $user_name, $audit_type, $record_name, $field_name, $old_values, $new_values, time(), $audit_msg);
          db_query("INSERT INTO {taxonomy_audit_trail}
                    (user_id, user_name, audit_type, record_name, field_name, old_value, new_value, record_inserted_on, audit_message)
                VALUES (%d, '%s', '%s', '%s', '%s', '%s', '%s', %d, '%s') ", $audit_args);

        break;

        case 'CHG' :
          if(array_key_exists('tag_name', $new_values)){
            $tag_name = $new_values['tag_name'];
          }else{
            $tag_name = $item_name;
          }
          if($new_values['tag_name'] != $old_values['tag_name'] && $new_values['synonyms'] != $old_values['synonyms']){
            $field_name = t('Name, Synonym');
          }elseif($new_values['tag_name'] == $old_values['tag_name'] && $new_values['synonyms'] != $old_values['synonyms']){
            $field_name = t('Synonym');
          }elseif($new_values['tag_name'] != $old_values['tag_name'] && $new_values['synonyms'] == $old_values['synonyms']){
            $field_name = t('Name');
          }

          $old_values = serialize($old_values);
          $new_values = serialize($new_values);
          $audit_msg = t('Tag !tag_name has been updated.', array('!tag_name' => $tag_name));
          $audit_args = array($user_id, $user_name, $audit_type, $record_name, $field_name, $old_values, $new_values, time(), $audit_msg);
          db_query("INSERT INTO {taxonomy_audit_trail}
                    (user_id, user_name, audit_type, record_name, field_name, old_value, new_value, record_inserted_on, audit_message)
                VALUES (%d, '%s', '%s', '%s', '%s', '%s', '%s', %d, '%s') ", $audit_args);

        break;
      }
    break;

    case 'Category':
      switch ($audit_type){

        case 'ADD':
          $cat_name = $new_values['category_name'];
          $field_name = t('Name, parent');
          $new_values = serialize($new_values);
          $audit_msg = t('Category !cat_name has been added.', array('!cat_name' => $cat_name));
          $audit_args = array($user_id, $user_name, $audit_type, $record_name, $field_name, $old_values, $new_values, time(), $audit_msg);
          db_query("INSERT INTO {taxonomy_audit_trail}
                    (user_id, user_name, audit_type, record_name, field_name, old_value, new_value, record_inserted_on, audit_message)
                VALUES (%d, '%s', '%s', '%s', '%s', '%s', '%s', %d, '%s') ", $audit_args);
        break;

        case 'CHG':
          if($new_values['category_name'] != $old_values['category_name'] && $new_values['parent'] != $old_values['parent']){
            $field_name = t('Name, Parent');
            $cat_name = $new_values['category_name'];
          }elseif($new_values['category_name'] == $old_values['category_name'] && $new_values['parent'] != $old_values['parent']){
            $field_name = t('parent');
            $cat_name = $item_name;

          }elseif($new_values['category_name'] != $old_values['category_name'] && $new_values['parent'] == $old_values['parent']){
            $field_name = t('Name');
            $cat_name = $new_values['category_name'];
          }
          $old_values = serialize($old_values);
          $new_values = serialize($new_values);
          $audit_msg = t('Category !cat_name has been updated.', array('!cat_name' => $cat_name));
          $audit_args = array($user_id, $user_name, $audit_type, $record_name, $field_name, $old_values, $new_values, time(), $audit_msg);
          db_query("INSERT INTO {taxonomy_audit_trail}
                    (user_id, user_name, audit_type, record_name, field_name, old_value, new_value, record_inserted_on, audit_message)
                VALUES (%d, '%s', '%s', '%s', '%s', '%s', '%s', %d, '%s') ", $audit_args);
        break;
      }
    break;
  }
}

/**
 * Function for export audit trail
 */
function export_taxonomy_audit_trail_filter(){

  $form = array();
  $form['date_fieldset'] = array(
    '#type' => 'fieldset',
    '#collapsible' => FALSE,
    '#collapsed' => FALSE,
  );
  $form['date_fieldset']['start_date'] = array(
    '#type' => 'date_popup',
    '#title' => t('Start Date'),
    '#default_value' => '',
    '#date_format' => 'd.m.Y',
    '#date_label_position' => 'within',
    '#date_year_range' => '-3:+3',
  );
  $form['date_fieldset']['end_date'] = array(
    '#type' => 'date_popup',
    '#title' => t('End Date'),
    '#default_value' => '',
    '#date_format' => 'd.m.Y',
    '#date_label_position' => 'within',
    '#date_year_range' => '-3:+3',
  );

  $form['buttons_fieldset'] = array(
    '#type' => 'fieldset',
    '#collapsible' => FALSE,
    '#collapsed' => FALSE,
  );

  $form['buttons_fieldset']['generate_repost'] = array(
    '#type' => 'submit',
    '#value' => t('Generate'),
  );

  return $form;
}


/**
 * validate function
 */
function export_taxonomy_audit_trail_filter_validate($form, &$form_state){

  if(!(isset($form_state['values']['start_date'])) || !(isset($form_state['values']['end_date'])) ) {
    form_set_error('', t('Start Date / End Date is required to generate audit trail.'));
  }

  if(isset($form_state['values']['start_date']) && isset($form_state['values']['end_date'])) {
    //Generate the Start Timestamp
    $start_time_stamp = strtotime($form_state['values']['start_date']);
    $m1 = date('m',$start_time_stamp);
    $d1 = date('d',$start_time_stamp);
    $y1 = date('Y',$start_time_stamp);
    $startdate = mktime(0, 0, 0, $m1, $d1, $y1);

    //Generate the End Timestamp
    $end_time_stamp = strtotime($form_state['values']['end_date']);
    $m2 = date('m',$end_time_stamp);
    $d2 = date('d',$end_time_stamp);
    $y2 = date('Y',$end_time_stamp);
    $enddate = mktime(23, 59, 59, $m2, $d2, $y2);
    if($startdate > $enddate) {
      form_set_error('', t('End date should be beyond the Start date.'));
    }

  }
}

/**
 * function for hook_submit
 */
function export_taxonomy_audit_trail_filter_submit($form, &$form_state){

  $sql_args = array();
  $startdate = 0;
  $enddate = 0;
  $where_clause = '';

  if($form_state['values']['start_date'] != ''){
    $start_time_stamp = strtotime($form_state['values']['start_date']);
      $m1 = date('m',$start_time_stamp);
      $d1 = date('d',$start_time_stamp);
      $y1 = date('Y',$start_time_stamp);
      $startdate = mktime(0, 0, 0, $m1, $d1, $y1);

  }

  if($form_state['values']['end_date'] != ''){
    $end_time_stamp = strtotime($form_state['values']['end_date']);
      $m2 = date('m',$end_time_stamp);
      $d2 = date('d',$end_time_stamp);
      $y2 = date('Y',$end_time_stamp);
      $enddate = mktime(23, 59, 59, $m2, $d2, $y2);
  }

  if($startdate > 0 && $enddate > 0){
    $where_clause.=" WHERE record_inserted_on BETWEEN %d AND %d";
    array_push($sql_args, $startdate);
    array_push($sql_args, $enddate);

  }elseif($startdate > 0 && $enddate == 0){
    $where_clause.=" WHERE record_inserted_on > %d ";
    array_push($sql_args, $startdate);
  } elseif($startdate == 0 && $enddate > 0){
    $where_clause.=" WHERE record_inserted_on < %d ";
    array_push($sql_args, $enddate);
  }

  $audit_res = db_query("SELECT user_id, user_name, audit_type, record_name, field_name, old_value, new_value, record_inserted_on, audit_message FROM {taxonomy_audit_trail} $where_clause ORDER BY id ASC ", $sql_args);

  if($audit_res->num_rows == 0){
    drupal_set_message(t('No recrods found.'));
  } else {
    $generate_report = TRUE;
    $old_value = array();
    $new_value = array();
    $csv_header = array(array('User id', 'User Name', 'Audit Type', 'Record Name', 'Field Name', 'Old Value', 'New Value', 'Date/Time', 'Audit Message'));
    while($audit_data = db_fetch_object($audit_res)){
      //$old_value = unserialize($audit_data->old_value),
      $csv_data[] = array(
        $audit_data->user_id,
        $audit_data->user_name,
        $audit_data->audit_type,
        $audit_data->record_name,
        $audit_data->field_name,
        implode("; ", unserialize($audit_data->old_value)),
        implode("; ", unserialize($audit_data->new_value)),
        date('d/m/Y H:i:s', $audit_data->record_inserted_on),
        $audit_data->audit_message,
      );
    }
    $csv = array_merge($csv_header, $csv_data); //Merging the Header & Body
  }

  if($generate_report === TRUE && $csv !== NULL) {
    drupal_set_header("Content-type: text/csv");
    drupal_set_header("Content-Disposition: attachment; filename=taxonomy_audit_record_" . date("Y-m-d-H-i-s") . ".csv");
    drupal_set_header("Pragma: no-cache");
    drupal_set_header("Expires: 0");
    outputCSV($csv);
    exit();
  }
}

/**
 * Function to get the tag name for a given synonym
 * @param  string $name
 * @return bool/string
 */
function _get_tag_name_by_synonym_name($name) {
  $name = filter_xss($name);
  $tag_name_sql = "SELECT TD.name FROM {term_data} AS TD INNER JOIN {term_synonym} AS TS ON TD.tid = TS.tid WHERE TS.name = '%s'";
  $tag_name_query = db_query($tag_name_sql, $name);
  if($tag_name_query->num_rows > 0) {
    $tag_name_data = db_fetch_object($tag_name_query);
    return $tag_name_data->name;
  }
  return NULL;
}

