// JavaScript Document
function http_post(url, data, type, success_func) {
	var ajax = $.ajax({
		type: type,
		url: url,
		timeout: 5000,
		data: data,
		success: success_func, 
		dataType: 'json',
		async: true,
	});
}

function blacklist_push(name) {
	var ajax = $.ajax({
		type: "POST",
		url: '/api/blacklist/push',
		data: {'name': name},
		success: function(data) {
			
			//alert(data);
			location.reload();
		}, 
		dataType: 'json',
		async: true,
	});
}

function blacklist_pop(name) {
	var ajax = $.ajax({
		type: "POST",
		url: '/api/blacklist/pop',
		data: {'name': name},
		success: function(data) {
			
			//alert(data);
			location.reload();
		}, 
		dataType: 'json',
		async: true,
	});
}

function yuming_follow(name) {
	var ajax = $.ajax({
		type: "POST",
		url: '/api/yuming/follow',
		data: {'name': name},
		success: function(data) {
			
			//alert(data);
			location.reload();
		}, 
		dataType: 'json',
		async: true,
	});
}

function yuming_unfollow(name) {
	var ajax = $.ajax({
		type: "POST",
		url: '/api/yuming/unfollow',
		data: {'name': name},
		success: function(data) {
			
			//alert(data);
			location.reload();
		}, 
		dataType: 'json',
		async: true,
	});
}
