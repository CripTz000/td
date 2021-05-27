//
// Copyright Aliaksei Levin (levlam@telegram.org), Arseny Smirnov (arseny30@gmail.com) 2014-2021
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#include "td/telegram/LinkManager.h"

#include "td/telegram/td_api.h"

#include "td/utils/common.h"
#include "td/utils/tests.h"

static void check_link(td::string url, td::string expected) {
  auto result = td::LinkManager::check_link(url);
  if (result.is_ok()) {
    ASSERT_STREQ(expected, result.ok());
  } else {
    ASSERT_TRUE(expected.empty());
  }
}

TEST(Link, check_link) {
  check_link("sftp://google.com", "");
  check_link("tg://google.com", "tg://google.com/");
  check_link("tOn://google", "ton://google/");
  check_link("httP://google.com?1#tes", "http://google.com/?1#tes");
  check_link("httPs://google.com/?1#tes", "https://google.com/?1#tes");
  check_link("tg://google?1#tes", "tg://google?1#tes");
  check_link("tg://google/?1#tes", "tg://google?1#tes");
  check_link("TG:_", "tg://_/");
  check_link("sftp://google.com", "");
  check_link("sftp://google.com", "");
  check_link("http:google.com", "");
  check_link("tg://http://google.com", "");
  check_link("tg:http://google.com", "");
  check_link("tg:https://google.com", "");
  check_link("tg:test@google.com", "");
  check_link("tg:google.com:80", "");
  check_link("tg:google.com", "tg://google.com/");
  check_link("tg:google.com:0", "");
  check_link("tg:google.com:a", "");
  check_link("tg:[2001:db8:0:0:0:ff00:42:8329]", "");
  check_link("tg:127.0.0.1", "tg://127.0.0.1/");
  check_link("http://[2001:db8:0:0:0:ff00:42:8329]", "http://[2001:db8:0:0:0:ff00:42:8329]/");
  check_link("http://localhost", "");
  check_link("http://..", "http://../");
  check_link("..", "http://../");
  check_link("https://.", "");
}

static void parse_internal_link(td::string url, td::td_api::object_ptr<td::td_api::InternalLinkType> expected) {
  // LOG(ERROR) << url;
  auto result = td::LinkManager::parse_internal_link(url);
  if (result != nullptr) {
    auto object = result->get_internal_link_type_object();
    if (object->get_id() == td::td_api::internalLinkTypeMessageDraft::ID) {
      static_cast<td::td_api::internalLinkTypeMessageDraft *>(object.get())->text_->entities_.clear();
    }
    ASSERT_STREQ(to_string(expected), to_string(object));
  } else {
    ASSERT_TRUE(expected == nullptr);
  }
}

TEST(Link, parse_internal_link) {
  auto authentication_code = [](td::string code) {
    return td::td_api::make_object<td::td_api::internalLinkTypeAuthenticationCode>(code);
  };
  auto background = [](td::string background_name) {
    return td::td_api::make_object<td::td_api::internalLinkTypeBackground>(background_name);
  };
  auto chat_invite_link = [] {
    return td::td_api::make_object<td::td_api::internalLinkTypeChatInviteLink>();
  };
  auto message = [] {
    return td::td_api::make_object<td::td_api::internalLinkTypeMessage>();
  };
  auto message_draft = [](td::string text, bool contains_url) {
    auto formatted_text = td::td_api::make_object<td::td_api::formattedText>();
    formatted_text->text_ = std::move(text);
    return td::td_api::make_object<td::td_api::internalLinkTypeMessageDraft>(std::move(formatted_text), contains_url);
  };
  auto qr_code_authentication = []() {
    return td::td_api::make_object<td::td_api::internalLinkTypeQrCodeAuthentication>();
  };
  auto unknown_deep_link = [] {
    return td::td_api::make_object<td::td_api::internalLinkTypeUnknownDeepLink>();
  };

  parse_internal_link("t.me/levlam/1", message());
  parse_internal_link("telegram.me/levlam/1", message());
  parse_internal_link("telegram.dog/levlam/1", message());
  parse_internal_link("www.t.me/levlam/1", message());
  parse_internal_link("www%2etelegram.me/levlam/1", message());
  parse_internal_link("www%2Etelegram.dog/levlam/1", message());
  parse_internal_link("www%252Etelegram.dog/levlam/1", nullptr);
  parse_internal_link("http://t.me/levlam/1", message());
  parse_internal_link("https://t.me/levlam/1", message());
  parse_internal_link("hTtp://www.t.me:443/levlam/1", message());
  parse_internal_link("httPs://t.me:80/levlam/1", message());
  parse_internal_link("https://t.me:200/levlam/1", nullptr);
  parse_internal_link("http:t.me/levlam/1", nullptr);
  parse_internal_link("t.dog/levlam/1", nullptr);
  parse_internal_link("t.m/levlam/1", nullptr);
  parse_internal_link("t.men/levlam/1", nullptr);

  parse_internal_link("tg:resolve?domain=username&post=12345&single", message());
  parse_internal_link("TG://resolve?domain=username&post=12345&single", message());
  parse_internal_link("TG://test@resolve?domain=username&post=12345&single", nullptr);
  parse_internal_link("tg:resolve:80?domain=username&post=12345&single", nullptr);
  parse_internal_link("tg:http://resolve?domain=username&post=12345&single", nullptr);
  parse_internal_link("tg:https://resolve?domain=username&post=12345&single", nullptr);
  parse_internal_link("tg:resolve?domain=&post=12345&single", unknown_deep_link());
  parse_internal_link("tg:resolve?domain=telegram&post=&single", unknown_deep_link());

  parse_internal_link("t.me/username/12345?single", message());
  parse_internal_link("t.me/username/12345?asdasd", message());
  parse_internal_link("t.me/username/12345", message());
  parse_internal_link("t.me/username/12345/", message());
  parse_internal_link("t.me/username/12345#asdasd", message());
  parse_internal_link("t.me/username/12345//?single", message());
  parse_internal_link("t.me/username/12345/asdasd//asd/asd/asd/?single", message());
  parse_internal_link("t.me/username/1asdasdas/asdasd//asd/asd/asd/?single", message());
  parse_internal_link("t.me/username/asd", nullptr);
  parse_internal_link("t.me/username/0", nullptr);
  parse_internal_link("t.me/username/-12345", nullptr);
  parse_internal_link("t.me//12345?single", nullptr);
  parse_internal_link("https://telegram.dog/telegram/?single", nullptr);

  parse_internal_link("tg:privatepost?domain=username/12345&single", unknown_deep_link());
  parse_internal_link("tg:privatepost?channel=username/12345&single", unknown_deep_link());
  parse_internal_link("tg:privatepost?channel=username&msg_id=12345", message());

  parse_internal_link("t.me/c/12345?single", nullptr);
  parse_internal_link("t.me/c/1/c?single", nullptr);
  parse_internal_link("t.me/c/c/1?single", nullptr);
  parse_internal_link("t.me/c//1?single", nullptr);
  parse_internal_link("t.me/c/12345/123?single", message());
  parse_internal_link("t.me/c/12345/123/asd/asd////?single", message());

  parse_internal_link("tg:bg?color=111111#asdasd", background("111111"));
  parse_internal_link("tg:bg?color=11111%31", background("111111"));
  parse_internal_link("tg:bg?color=11111%20", background("11111%20"));
  parse_internal_link("tg:bg?gradient=111111-222222", background("111111-222222"));
  parse_internal_link("tg:bg?rotation=180%20&gradient=111111-222222%20",
                      background("111111-222222%20?rotation=180%20"));
  parse_internal_link("tg:bg?gradient=111111~222222", background("111111~222222"));
  parse_internal_link("tg:bg?gradient=abacaba", background("abacaba"));
  parse_internal_link("tg:bg?slug=111111~222222#asdasd", background("111111~222222"));
  parse_internal_link("tg:bg?slug=111111~222222&mode=12", background("111111~222222?mode=12"));
  parse_internal_link("tg:bg?slug=111111~222222&mode=12&text=1", background("111111~222222?mode=12"));
  parse_internal_link("tg:bg?slug=111111~222222&mode=12&mode=1", background("111111~222222?mode=12"));
  parse_internal_link("tg:bg?slug=test&mode=12&rotation=4&intensity=2&bg_color=3",
                      background("test?mode=12&intensity=2&bg_color=3&rotation=4"));
  parse_internal_link("tg:bg?mode=12&&slug=test&intensity=2&bg_color=3",
                      background("test?mode=12&intensity=2&bg_color=3"));
  parse_internal_link("tg:bg?mode=12&intensity=2&bg_color=3", unknown_deep_link());

  parse_internal_link("tg:bg?color=111111#asdasd", background("111111"));
  parse_internal_link("tg:bg?color=11111%31", background("111111"));
  parse_internal_link("tg:bg?color=11111%20", background("11111%20"));
  parse_internal_link("tg:bg?gradient=111111-222222", background("111111-222222"));
  parse_internal_link("tg:bg?rotation=180%20&gradient=111111-222222%20",
                      background("111111-222222%20?rotation=180%20"));
  parse_internal_link("tg:bg?gradient=111111~222222", background("111111~222222"));
  parse_internal_link("tg:bg?gradient=abacaba", background("abacaba"));
  parse_internal_link("tg:bg?slug=111111~222222#asdasd", background("111111~222222"));
  parse_internal_link("tg:bg?slug=111111~222222&mode=12", background("111111~222222?mode=12"));
  parse_internal_link("tg:bg?slug=111111~222222&mode=12&text=1", background("111111~222222?mode=12"));
  parse_internal_link("tg:bg?slug=111111~222222&mode=12&mode=1", background("111111~222222?mode=12"));
  parse_internal_link("tg:bg?slug=test&mode=12&rotation=4&intensity=2&bg_color=3",
                      background("test?mode=12&intensity=2&bg_color=3&rotation=4"));
  parse_internal_link("tg:bg?mode=12&&slug=test&intensity=2&bg_color=3",
                      background("test?mode=12&intensity=2&bg_color=3"));
  parse_internal_link("tg:bg?mode=12&intensity=2&bg_color=3", unknown_deep_link());

  parse_internal_link("%54.me/bg/111111#asdasd", background("111111"));
  parse_internal_link("t.me/bg/11111%31", background("111111"));
  parse_internal_link("t.me/bg/11111%20", background("11111%20"));
  parse_internal_link("t.me/bg/111111-222222", background("111111-222222"));
  parse_internal_link("t.me/bg/111111-222222%20?rotation=180%20", background("111111-222222%20?rotation=180%20"));
  parse_internal_link("t.me/bg/111111~222222", background("111111~222222"));
  parse_internal_link("t.me/bg/abacaba", background("abacaba"));
  parse_internal_link("t.me/Bg/abacaba", nullptr);
  parse_internal_link("t.me/bg/111111~222222#asdasd", background("111111~222222"));
  parse_internal_link("t.me/bg/111111~222222?mode=12", background("111111~222222?mode=12"));
  parse_internal_link("t.me/bg/111111~222222?mode=12&text=1", background("111111~222222?mode=12"));
  parse_internal_link("t.me/bg/111111~222222?mode=12&mode=1", background("111111~222222?mode=12"));
  parse_internal_link("t.me/bg/test?mode=12&rotation=4&intensity=2&bg_color=3",
                      background("test?mode=12&intensity=2&bg_color=3&rotation=4"));
  parse_internal_link("t.me/%62g/test/?mode=12&&&intensity=2&bg_color=3",
                      background("test?mode=12&intensity=2&bg_color=3"));
  parse_internal_link("t.me/bg//", nullptr);
  parse_internal_link("t.me/bg/%20/", background("%20"));
  parse_internal_link("t.me/bg/", nullptr);

  parse_internal_link("tg:share?url=google.com&text=text#asdasd", message_draft("google.com\ntext", true));
  parse_internal_link("tg:share?url=google.com&text=", message_draft("google.com", false));
  parse_internal_link("tg:share?url=&text=google.com", message_draft("google.com", false));
  parse_internal_link("tg:msg_url?url=google.com&text=text", message_draft("google.com\ntext", true));
  parse_internal_link("tg:msg_url?url=google.com&text=", message_draft("google.com", false));
  parse_internal_link("tg:msg_url?url=&text=google.com", message_draft("google.com", false));
  parse_internal_link("tg:msg?url=google.com&text=text", message_draft("google.com\ntext", true));
  parse_internal_link("tg:msg?url=google.com&text=", message_draft("google.com", false));
  parse_internal_link("tg:msg?url=&text=google.com", message_draft("google.com", false));
  parse_internal_link("tg:msg?url=&text=\n\n\n\n\n\n\n\n", nullptr);
  parse_internal_link("tg:msg?url=%20\n&text=", nullptr);
  parse_internal_link("tg:msg?url=%20\n&text=google.com", message_draft("google.com", false));
  parse_internal_link("tg:msg?url=@&text=", message_draft(" @", false));
  parse_internal_link("tg:msg?url=&text=@", message_draft(" @", false));
  parse_internal_link("tg:msg?url=@&text=@", message_draft(" @\n@", true));
  parse_internal_link("tg:msg?url=%FF&text=1", nullptr);

  parse_internal_link("https://t.me/share?url=google.com&text=text#asdasd", message_draft("google.com\ntext", true));
  parse_internal_link("https://t.me/share?url=google.com&text=", message_draft("google.com", false));
  parse_internal_link("https://t.me/share?url=&text=google.com", message_draft("google.com", false));
  parse_internal_link("https://t.me/msg?url=google.com&text=text", message_draft("google.com\ntext", true));
  parse_internal_link("https://t.me/msg?url=google.com&text=", message_draft("google.com", false));
  parse_internal_link("https://t.me/msg?url=&text=google.com", message_draft("google.com", false));
  parse_internal_link("https://t.me/msg?url=google.com&text=text", message_draft("google.com\ntext", true));
  parse_internal_link("https://t.me/msg?url=google.com&text=", message_draft("google.com", false));
  parse_internal_link("https://t.me/msg?url=&text=google.com", message_draft("google.com", false));
  parse_internal_link("https://t.me/msg?url=&text=\n\n\n\n\n\n\n\n", nullptr);
  parse_internal_link("https://t.me/msg?url=%20\n&text=", nullptr);
  parse_internal_link("https://t.me/msg?url=%20\n&text=google.com", message_draft("google.com", false));
  parse_internal_link("https://t.me/msg?url=@&text=", message_draft(" @", false));
  parse_internal_link("https://t.me/msg?url=&text=@", message_draft(" @", false));
  parse_internal_link("https://t.me/msg?url=@&text=@", message_draft(" @\n@", true));
  parse_internal_link("https://t.me/msg?url=%FF&text=1", nullptr);

  parse_internal_link("tg:login?codec=12345", unknown_deep_link());
  parse_internal_link("tg:login", unknown_deep_link());
  parse_internal_link("tg:login?code=abacaba", authentication_code("abacaba"));
  parse_internal_link("tg:login?code=123456", authentication_code("123456"));

  parse_internal_link("t.me/login?codec=12345", nullptr);
  parse_internal_link("t.me/login", nullptr);
  parse_internal_link("t.me/login/", nullptr);
  parse_internal_link("t.me/login//12345", nullptr);
  parse_internal_link("t.me/login?/12345", nullptr);
  parse_internal_link("t.me/login/?12345", nullptr);
  parse_internal_link("t.me/login/#12345", nullptr);
  parse_internal_link("t.me/login/abacaba", authentication_code("abacaba"));
  parse_internal_link("t.me/login/aba%20aba", authentication_code("aba aba"));
  parse_internal_link("t.me/login/123456a", authentication_code("123456a"));
  parse_internal_link("t.me/login/12345678901", authentication_code("12345678901"));
  parse_internal_link("t.me/login/123456", authentication_code("123456"));
  parse_internal_link("t.me/login/123456/123123/12/31/a/s//21w/?asdas#test", authentication_code("123456"));

  parse_internal_link("tg:login?token=abacaba", qr_code_authentication());
  parse_internal_link("tg:login?token=", unknown_deep_link());

  parse_internal_link("t.me/joinchat?invite=abcdef", nullptr);
  parse_internal_link("t.me/joinchat", nullptr);
  parse_internal_link("t.me/joinchat/", nullptr);
  parse_internal_link("t.me/joinchat//abcdef", nullptr);
  parse_internal_link("t.me/joinchat?/abcdef", nullptr);
  parse_internal_link("t.me/joinchat/?abcdef", nullptr);
  parse_internal_link("t.me/joinchat/#abcdef", nullptr);
  parse_internal_link("t.me/joinchat/abacaba", chat_invite_link());
  parse_internal_link("t.me/joinchat/aba%20aba", chat_invite_link());
  parse_internal_link("t.me/joinchat/123456a", chat_invite_link());
  parse_internal_link("t.me/joinchat/12345678901", chat_invite_link());
  parse_internal_link("t.me/joinchat/123456", chat_invite_link());
  parse_internal_link("t.me/joinchat/123456/123123/12/31/a/s//21w/?asdas#test", chat_invite_link());

  parse_internal_link("t.me/+?invite=abcdef", nullptr);
  parse_internal_link("t.me/+a", chat_invite_link());
  parse_internal_link("t.me/+", nullptr);
  parse_internal_link("t.me/+/abcdef", nullptr);
  parse_internal_link("t.me/ ?/abcdef", nullptr);
  parse_internal_link("t.me/+?abcdef", nullptr);
  parse_internal_link("t.me/+#abcdef", nullptr);
  parse_internal_link("t.me/ abacaba", chat_invite_link());
  parse_internal_link("t.me/+aba%20aba", chat_invite_link());
  parse_internal_link("t.me/+123456a", chat_invite_link());
  parse_internal_link("t.me/%2012345678901", chat_invite_link());
  parse_internal_link("t.me/+123456", chat_invite_link());
  parse_internal_link("t.me/ 123456/123123/12/31/a/s//21w/?asdas#test", chat_invite_link());
  parse_internal_link("t.me/ /123456/123123/12/31/a/s//21w/?asdas#test", nullptr);

  parse_internal_link("tg:join?invite=abcdef", chat_invite_link());
  parse_internal_link("tg:join?invite=", unknown_deep_link());
}
