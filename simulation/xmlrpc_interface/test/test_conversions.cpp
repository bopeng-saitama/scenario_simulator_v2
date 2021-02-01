// Copyright 2015-2020 Tier IV, Inc. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <gtest/gtest.h>

#include <xmlrpc_interface/conversions.hpp>

#include <string>

TEST(Conversion, ConvertInitializeResponse)
{
  simulation_api_schema::InitializeResponse res;
  res.mutable_result()->set_success(true);
  res.mutable_result()->set_description("test");
  XmlRpc::XmlRpcValue xml;
  EXPECT_NO_THROW(xmlrpc_interfae::fromProto(res, xml));
  std::string description = xml[xmlrpc_interfae::key_description];
  EXPECT_STREQ(description.c_str(), "test");
  res.mutable_result()->set_description("");
  EXPECT_NO_THROW(xmlrpc_interfae::toProto(xml, res));
  EXPECT_STREQ(res.result().description().c_str(), "test");
  std::string serialized_str = "";
  res.SerializeToString(&serialized_str);
  EXPECT_STREQ(
    xmlrpc_interfae::serialize<simulation_api_schema::InitializeResponse>(xml).c_str(),
    serialized_str.c_str());
}

TEST(Conversion, ConvertInitializeRequest)
{
  simulation_api_schema::InitializeRequest req;
  req.set_realtime_factor(0.1);
  req.set_step_time(0.5);
  XmlRpc::XmlRpcValue xml;
  xmlrpc_interfae::fromProto(req, xml);
  EXPECT_DOUBLE_EQ(req.step_time(), xml[xmlrpc_interfae::key_step_time]);
  EXPECT_DOUBLE_EQ(req.realtime_factor(), xml[xmlrpc_interfae::key_realtime_factor]);
  req.set_realtime_factor(0);
  req.set_step_time(0);
  xmlrpc_interfae::toProto(xml, req);
  EXPECT_DOUBLE_EQ(req.step_time(), xml[xmlrpc_interfae::key_step_time]);
  EXPECT_DOUBLE_EQ(req.realtime_factor(), xml[xmlrpc_interfae::key_realtime_factor]);
}

int main(int argc, char ** argv)
{
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
