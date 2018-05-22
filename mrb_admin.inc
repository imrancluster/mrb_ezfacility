<?php

/**
* @file
* mrb_ezfacility.inc
*/


/**
 * Callback function for start-mrb-pulling
 */
function start_meeting_room_booking_pulling() {

    if (isset($_GET['start_date']) && isset($_GET['end_date'])) {

        $start_date = filter_xss($_GET['start_date']);
        $end_date = filter_xss($_GET['end_date']);

        $mrb_data = call_ez_facility_for_mrb($start_date, $end_date);

        if (!empty($mrb_data)) {

            $previous_mrb = _get_previous_mrb($start_date, $end_date);

            $new = 0;
            $update = 0;
            foreach ($mrb_data as $mrb) {
                if (array_key_exists($mrb['ResourceID'], $previous_mrb)) {
                    // Update existing mrb
                    _update_existing_mrb($previous_mrb[$mrb['ResourceID']], $mrb);
                    $update++;
                } else {
                    // Create new mrb
                    _create_new_mrb($mrb);
                    $new++;
                }

            }

            return theme( 'mrb_operation_result',
                array(
                    'data' => [
                        'new' => $new,
                        'update' => $update,
                        'start_date' => $start_date,
                        'end_date' => $end_date,
                    ]
                )
            );
        } else {
            return "No MRB found from <strong>" .$start_date . " (mm/dd/yyyy)</strong> to <strong>" . $end_date . " (mm/dd/yyyy)</strong>.";
        }
    }

    return "<strong>start_date (mm/dd/yyyy)</strong> and <strong>end_date (mm/dd/yyyy)</strong> get parameters were required.";
}

/**
 * @param $stat_date
 * @param $end_date
 * @return array
 */
function _get_previous_mrb($stat_date, $end_date) {

    $input_start_date = date('Y-m-d H:i:s', strtotime($stat_date));
    $input_end_date = date('Y-m-d H:i:s', strtotime($end_date));

    $query = db_select('node', 'n');
    $query->leftJoin('field_data_field_schedule_date', 'sd', 'n.nid = sd.entity_id');
    $query->leftJoin('field_data_field_ez_facility_id', 'fi', 'n.nid = fi.entity_id');
    $query->fields('n', array('nid'));
    $query->fields('sd', array('field_schedule_date_value', 'field_schedule_date_value2'));
    $query->addField('fi', 'field_ez_facility_id_value', 'facility_id');
    $query->condition('n.type', 'meeting_room_booking');
    $or = db_or();
    $or->condition('sd.field_schedule_date_value',$input_start_date, '>=');
    $or->condition('sd.field_schedule_date_value2',$input_end_date, '<=');
    $query->condition($or);
    $results = $query->execute();


    $previous_mrb = array();
    foreach($results as $result) {
        $previous_mrb[$result->facility_id] = $result;
    }

    return $previous_mrb;
}

/**
 * @param $mrb
 * @throws Exception
 */
function _create_new_mrb($mrb) {
    $node = new stdClass();
    $node->title = $mrb['Title'];
    $node->type = "meeting_room_booking";
    node_object_prepare($node);
    $node->field_ez_facility_id['und'][0]['value'] = $mrb['ResourceID'];

    $item_date_start = gmdate('Y-m-d G:i:s', strtotime($mrb['StartDateTime']));
    $item_date_end = gmdate('Y-m-d G:i:s', strtotime($mrb['EndDateTime']));

    $node->field_schedule_date['und'][0] = array(
        'value' => $item_date_start,
        'value2' => $item_date_end,
        'timezone' => 'UTC',
        'timezone_db' => 'UTC',
    );

    $node->field_location['und'][0]['target_id'] = _get_resource_reference($mrb['Resource']);

    $node->uid = 1;
    $node->status = 1;
    $node->language = LANGUAGE_NONE;
    $node = node_submit($node);
    node_save($node);
}

/**
 * @param $existing_mrb
 * @param $mrb
 * @throws Exception
 */
function _update_existing_mrb($existing_mrb, $mrb) {

    $node = node_load($existing_mrb->nid);

    if ($node) {
        $node->title = $mrb['Title'];
        $item_date_start = gmdate('Y-m-d G:i:s', strtotime($mrb['StartDateTime']));
        $item_date_end = gmdate('Y-m-d G:i:s', strtotime($mrb['EndDateTime']));

        $node->field_schedule_date['und'][0] = array(
            'value' => $item_date_start,
            'value2' => $item_date_end,
            'timezone' => 'UTC',
            'timezone_db' => 'UTC',
        );
        $node->field_location['und'][0]['target_id'] = _get_resource_reference($mrb['Resource']);
        node_save($node);
    }
}

/**
 * @param $resource
 * @return int
 * @throws Exception
 */
function _get_resource_reference($resource) {

    $resource_id = _get_resource_id_by_title($resource);

    if ( $resource_id != 0) {
        return $resource_id;
    }

    _create_new_resouce($resource);
    return _get_resource_id_by_title($resource);
}

/**
 * @param $title
 * @return int
 */
function _get_resource_id_by_title($title) {
    $query = db_select('node', 'n');
    $query->fields('n', array('nid', 'title'))
        ->condition('title', $title);
    $result = $query->execute()->fetchAssoc();

    if ($result) {
        return $result['nid'];
    }

    return 0;
}

/**
 * @param $title
 * @throws Exception
 */
function _create_new_resouce($title) {
    $node = new stdClass();
    $node->title = $title;
    $node->type = "ez_facility_resource";
    node_object_prepare($node);
    $node->uid = 1;
    $node->status = 1;
    $node->language = LANGUAGE_NONE;
    $node = node_submit($node);
    node_save($node);
}

/**
 * @param string $STARTDATE
 * @param string $ENDDATE
 * @param int $CALID
 * @return array
 */
function call_ez_facility_for_mrb($STARTDATE='', $ENDDATE='', $CALID=9906) {

    $URL = 'http://tms.ezfacility.com/webservices/reservations.asmx/GetReservations';
    $USERID = 105480;
    $ReservationTypeID = 182573;

    if (!$STARTDATE) {
        $STARTDATE = date('m/d/Y');
        $ENDDATE= date('m/t/Y');
    }

    if (!$ENDDATE) {
        $ENDDATE= date('m/t/Y',strtotime($STARTDATE));
    }

    $fields = array(
        'UserId' => $USERID,
        'CalendarTypeID' => $CALID,
        'StartDate' => $STARTDATE,
        'EndDate' => $ENDDATE
    );

    $URL .= '?' . http_build_query($fields);
    $result = file_get_contents($URL);

    $mrb_data = array();

    if (!empty($result)) {
        $results = json_decode($result,true);

        foreach ($results as $result) {
            if ($result['ReservationTypeID'] == $ReservationTypeID) {
                $mrb_data[] = $result;
            }

        }
    }

    return $mrb_data;
}