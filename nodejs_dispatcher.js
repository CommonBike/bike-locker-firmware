const UP_GET_SERVER_TIME=1,
      UP_INVOKE_DOWNLINK=2,
      UP_SET_LOCKER_STATE=3,
      UP_CHECK_IN=4,
      UP_CHECK_OUT=5;

const DOWN_SEND_SERVER_TIME=1,
	  DOWN_SEND_UNLOCK_CMD=2,
	  DOWN_RESERVE_LOCKER=3,
	  DOWN_SEND_WHITE_LIST_ITEM=4;

function getDate()
{
  var d = new Date();
  return d.getFullYear() + '-' +
  	     ((d.getMonth()<9)?'0':'') + (d.getMonth()+1) + '-' +
  	     ((d.getDate()<10)?'0':'') + d.getDate() + ' ' +
  		 ((d.getHours()<10)?'0':'') + d.getHours() + ':' +
		 ((d.getMinutes()<10)?'0':'') + d.getMinutes() + ':' +
		 ((d.getSeconds()<10)?'0':'') + d.getSeconds();
}

function getRandomByte()
{
  return Math.floor(Math.random()*0xff);
}

function log(msg,level)
{
  if (level===undefined)
  	level='INFO';
  console.log(getDate()+ ' ['+level+'] - ' + msg);
}

function sendTime(devEUI)
{
  var payload = new Buffer(7);
  var d = new Date();
  payload[0]=DOWN_SEND_SERVER_TIME;
  payload[1]=d.getFullYear()-2000;
  payload[2]=d.getMonth()+1;
  payload[3]=d.getDate();
  payload[4]=d.getHours();
  payload[5]=d.getMinutes();
  payload[6]=d.getSeconds();
  client.downlink(devEUI, payload,'1m',DOWN_SEND_SERVER_TIME);
  log('Sent current time stamp.');
}

function sendUnlock(devEUI)
{
  var payload = Buffer.allocUnsafe(2);
  payload[0]=DOWN_SEND_UNLOCK_CMD;
  payload[1]=getRandomByte();
  payload[2]=getRandomByte();
  client.downlink(devEUI, payload, '2m',DOWN_SEND_UNLOCK_CMD);
  log('Sent locker unlock command.');
}

function sendReservation(devEUI,cardId,validMinutes)
{
  var payload = Buffer.allocUnsafe(11);
  var d = new Date();
  d.setUTCMinutes(d.getUTCMinutes()+validMinutes);
  payload[0]=DOWN_RESERVE_LOCKER;
  payload[1]=d.getFullYear()-2000;
  payload[2]=d.getMonth()+1;
  payload[3]=d.getDate();
  payload[4]=d.getHours();
  payload[5]=d.getMinutes();
  payload[6]=d.getSeconds();
  payload[7]=cardId[0];
  payload[8]=cardId[1];
  payload[9]=cardId[2];
  payload[10]=cardId[3];
  client.downlink(devEUI, payload, '2h',DOWN_RESERVE_LOCKER);
  log('Send locker reservation command.');
}

function decodeTimeStamp(payload,offset)
{
  var time = '20';
  if (payload[offset]<10)
	time+='0';
  time+=payload[offset].toString(10);
  time+='-';
  if (payload[offset+1]<10)
	time+='0';
  time+=payload[offset+1].toString(10);
  time+='-';
  if (payload[offset+2]<10)
	time+='0';
  time+=payload[offset+2].toString(10);
  time+=' ';
  if (payload[offset+3]<10)
	time+='0';
  time+=payload[offset+3].toString(10);
  time+=':';
  if (payload[offset+4]<10)
	time+='0';
  time+=payload[offset+4].toString(10);
  time+=':';
  if (payload[offset+5]<10)
	time+='0';
  time+=payload[offset+5].toString(10);
  return time;
}

function decodeCardId(payload,offset)
{
  var cardId = '';
  for (var i=offset;i<(offset+4);i++)
  {
    if (payload[i]<16)
      cardId+='0';
    cardId+=payload[i].toString(16);
  }
  return cardId;
}

function dispatcher(msg)
{
  var cardId='';
  var timeStamp='';
  var lockerState='';
  var decodedMsg='';

  log('Message received on port:'+msg.port);
  switch(msg.port)
  {
    case UP_GET_SERVER_TIME:
      decodedMsg='GetTime()';
  	  log(decodedMsg);
	  sendTime(msg.devEUI);
      break;
    case UP_INVOKE_DOWNLINK:
      decodedMsg='InvokeDownlink()';
  	  log(decodedMsg);
	  // invoked if the button is pressed on the locker
	  // if we want to talk to the locker we can do it here
	  // for now we send a standard reservation
	  var id = new Buffer([0x01,0x02,0x03,0x04]);
	  sendReservation(msg.devEUI,id,3);
      //sendUnlock(msg.devEUI);
      break;
    case UP_SET_LOCKER_STATE:
      if (msg.fields.payload.length==11)
      {
        timeStamp=decodeTimeStamp(msg.fields.payload,0);
        cardId=decodeCardId(msg.fields.payload,6);

        switch(msg.fields.payload[10])
        {
          case 0:
      	    lockerState='vrij';
      	    break;
      	  case 1:
      	    lockerState='bezet';
	  	    break;
      	  case 2:
      	    lockerState='geblokkeerd';
	        break;
      	  case 3:
      	    lockerState='gereserveerd';
	        break;
      	  case 4:
      	    lockerState='buiten werking';
	        break;
      	  default:
      	    lockerState='unknown';
	        break;
        }

      	decodedMsg='SetLockerState(\''+timeStamp+'\',\''+cardId+'\',\''+lockerState+'\')';
  	  	log(decodedMsg);
	  	// update the database here
	  }
	  else
	  {
		log('Invalid packet length for UP_SET_LOCKER_STATE.','ERROR');
	  }
      break;
    case UP_CHECK_IN:
      if (msg.fields.payload.length==10)
      {
        timeStamp=decodeTimeStamp(msg.fields.payload,0);
        cardId=decodeCardId(msg.fields.payload,6);

      	decodedMsg='CheckIn(\''+timeStamp+'\',\''+cardId+'\')';
  	  	log(decodedMsg);
	  	// update the database here
	  }
	  else
	  {
		log('Invalid packet length for UP_CHECK_IN.','ERROR');
	  }
      break;
    case UP_CHECK_OUT:
      if (msg.fields.payload.length==10)
      {
        timeStamp=decodeTimeStamp(msg.fields.payload,0);
        cardId=decodeCardId(msg.fields.payload,6);

      	decodedMsg='CheckOut(\''+timeStamp+'\',\''+cardId+'\')';
  	  	log(decodedMsg);
	  	// update the database here
	  }
	  else
	  {
		log('Invalid packet length for UP_CHECK_OUT.','ERROR');
	  }
      break;
    default:
      decodedMsg='Unknown command received from TTN backend.';
  	  log(decodedMsg,'WARNING');
      break;
  }
}


var ttn = require('ttn');
var appEUI = '70B3D57ED0000C7C';
var accessKey = '3VdvpmrjiA68956AkqyIFGnKRr/Dmb5wKsgAb2eYQBQ=';
var client = new ttn.Client('staging.thethingsnetwork.org', appEUI, accessKey);

client.on('connect', function() {
  log('Connected to TTN backend.');
});

client.on('error', function (err) {
  log(err.message,'ERROR');
});

client.on('activation', function (e) {
  log('Device ' + e.devEUI + ' activated.');
});

client.on('uplink', function (msg) {
  log('Data received from TTN backend: ' + msg.devEUI + ', cnt: ' +msg.counter +', rssi: '+msg.metadata.rssi);
  dispatcher(msg);
});
