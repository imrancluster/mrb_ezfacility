<div class="screen-page-block">
    <h3>Available Events</h3>

    <ul>
        <?php if(isset($data['events']) && !empty($data['events'])): ?>
        <?php foreach($data['events'] as $event): ?>
            <li>
                <div class="title"><?php print $event['title']; ?></div>
                <p>Event start from <?php print $event['date']['start_date']; ?> to <?php print $event['date']['end_date']; ?></p>
                <hr>
            </li>
        <?php endforeach; ?>
        <?php else: ?>
            
        <?php endif; ?>
    </ul>

</div>